#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <iomanip>
#include "mpi.h"
#include <climits>

#include "Field.h"
#include "FieldStack.h"

using namespace std;

/* globals and defines */
#define BUFFER_SIZE 1024 // <= 1KB aby se posilalo neblokujicim zpusobem
#define WORK_BUFFER_SIZE 5120

#define MSG_WORK_REQUEST 1000
#define MSG_WORK_SENT    1001
#define MSG_WORK_NOWORK  1002
#define MSG_TOKEN        1003
#define MSG_FINISH       1004
#define MSG_SOLUTION     1005

const char *fileName = "input.txt"; // nepsi by bylo pomoci define protoze to potrebuju jen na zacatku, idealni aby normalne cetl ze vstupu a jen prepinacem ze souboru
bool verbose = false; // prepinac -v
bool verboseStackSize = false; // prepinac -vs
bool verboseProcessCommunication = true; // prepinac -vc

#define MASTER 0
int myID = -1; // od 0 do noIDs-1
string myPrefix = "";
int noIDs = -1;

/* Read data from given file and initialize given field. */
void initField(Field* &field, const char* fileName) {
    int a, b; // a, b - dimension of field
    int n; // number of non-zero numbers
    string dummy;
    ifstream in;

    // open file
    in.open(fileName);
    if (!in.is_open()) {
        ostringstream ss;
        ss << "Could not open a file \"" << fileName << "\"!";
        throw ss.str();
    }

    // read parameters
    in >> a >> b; // size of field
    getline(in, dummy); // get rid of new line character
    in >> n; // number of rectangles within the field
    getline(in, dummy); // get rid of new line character

    // create new field and fill with data
    field = new Field(Vector2D(a, b));
    field->fill(in);

    in.close();

    // check
    if (n != 0 && n != field->getRectangles()->getSize()) { // n = 0 <=> n is not given
        ostringstream ss;
        ss << "Given n (number of non-zero numbers) does not correspond to actual non-zero numbers in given field! " << "n=" << n << " != " << field->getRectangles()->getSize();
        throw ss.str();
    }
    if (field->getDimension().getX() * field->getDimension().getY() != field->getRectangles()->getAreaSum()) {
        ostringstream ss;
        ss << "Given rectangles areas (non-zero numbers) do not cover field area precisely! " << Vector2D(a, b).toDimensionString() << "=" << a * b << " != " << field->getRectangles()->getAreaSum();
        throw ss.str();
    }
}

/* Read arguements and process them */
void processArguments(int argc, char** argv) {
    for (int i = 1; i < argc; i++) { // first argument (argv[0]) is executable name
        if (strcmp(argv[i], "-v") == 0) {
            verbose = true;
        } else if (strcmp(argv[i], "-vs") == 0) {
            verboseStackSize = true;
        } else if (strcmp(argv[i], "-vc") == 0) {
            verboseProcessCommunication = true;
        } else if (strcmp(argv[i], "-f") == 0) {
            fileName = argv[i + 1];
            i++;
        } else if (strcmp(argv[i], "-h") == 0) {
            cout << "Usage:" << endl <<
                    "\t-v\t\tfor verbose" << endl <<
                    "\t-vs\t\tfor verbose stack (pushing and popping)" <<
                    "\t-f \"file\"\tto specific input file, default is \"input.txt\"" << endl <<
                    "\t-h\t\tfor this help" << endl;
            exit(EXIT_SUCCESS); // clean-up is not necessary, no class object has been created yet
        } else {
            ostringstream ss;
            ss << argv[i] << " - no such argument!";
            throw ss.str();
        }
    }
}

/**
 * 
 * $ ./generator a b n > gen.txt; ./transform.sh a b n gen.txt > trans.txt; ./ppr -f trans.txt
 * 
 */
int main(int argc, char** argv) {
    FieldStack* stack = new FieldStack(); // use an implicit constructor to initialise stack pointers & size
    Field* field = NULL;
    Field* bestField = NULL;

    double t_start, t_end;
    char buffer[BUFFER_SIZE];
    int pos = 0;
    char dummyBuffer;
    MPI_Status status;

    bool firstSendExecuted = false; // true after first send from Master to Slaves is executed
    bool workSentToLower = false; // true if Pj sent data to Pi where i < j, false after token sent
    bool haveToken = false;

    /* start up MPI */
    MPI_Init(&argc, &argv);

    /* find out process rank */
    MPI_Comm_rank(MPI_COMM_WORLD, &myID);

    stringstream tmp; // process prefix
    for (int i = 0; i <= myID; i++) {
        tmp << "    ";
    }
    tmp << myID << ": ";
    myPrefix = tmp.str();

    /* find out number of processes */
    MPI_Comm_size(MPI_COMM_WORLD, &noIDs);

    /* waiting for all process, then start */
    MPI_Barrier(MPI_COMM_WORLD);

    /* initialize computation */
    if (myID == MASTER) { // master load input data and create first Field
        /* time measuring - start */
        t_start = MPI_Wtime();

        // inicializace
        try {
            processArguments(argc, argv);
            initField(field, fileName);
        } catch (string ex) {
            cout << "Exception: " << ex << endl;

            delete field; // clean-up
            delete bestField; // clean-up
            delete stack; // clean-up
            exit(EXIT_FAILURE);
        }

        cout << "-------------------- TASK --------------------" << endl;
        cout << field->toString();
        cout << "-------------------- /TASK --------------------" << endl;

    } else { // slaves wait (blocking way) for first data from master
        if (verbose || verboseProcessCommunication) cout << myPrefix << "Waiting for start Field." << endl;

        MPI_Recv(buffer, BUFFER_SIZE, MPI_PACKED, MASTER, MSG_WORK_SENT, MPI_COMM_WORLD, &status);
        pos = 0;
        field = Field::unpack(buffer, BUFFER_SIZE, &pos);

        if (verbose || verboseProcessCommunication) cout << myPrefix << "Recieved start Field." << endl;
    }


    /* PARALELNY ALGORITMUS PODLA EDUXU */
    /*
     * pesek white = color unchanged
     * pesek black = color has been changed
     * 
     * master provides some work for every CPU and sends initial white pesek
     * for all fields in the local stack
     * {
     *    for current DFS
     *    {
     *       process for all shapes x,y
     *       process for all positions
     *    }
     * 	  stack->pop (ask for new field from the stack)
     *    for every n-th occurrence (n in between 10..100)
     *    {
     * 	     MPI_Iprobe (verify no message has been received)
     * 		 if (message received)
     *       {
     * 		    case (asked to provide work for another CPU)
     * 			{
     * 				if (my stack contains any element of level i am allow to send) // napriklad posledních k levelu nema cenu pprotoze jsou to uz opravdu maly ukoly (DEFINE a experimentovat)
     * 				{
     * 					divideStack()
     * 					MPI_Send (new Stack)
     * 				}
     * 				else
     * 				{
     * 					stack cannot be divided
     * 					MPI_Send (try another CPU)
     * 				}
     * 			}
     * 			case (pesek received)
     * 			{
     * 				if (master)
     * 				{
     * 					if (pesek white)
     * 					{
     * 						process rest of my stack
     * 						MPI_Send (finalise the end of computation for all other CPUs)
     * 					}
     * 					else (pesek black)
     * 					{
     * 						MPI_Send (new pesek to all CPUs of white color)
     * 					}
     * 				}
     * 				else (non-master)
     * 				{
     *     				if (local stack empty && received pesek is white)
     * 						MPI_Send (white pesek)
     * 					else (=my stack is not empty || another CPU has already changed pesek to black)
     * 						MPI_Send (black pesek) 
     * 				}
     * 			}
     * 			case (finish detected by P0)
     * 			{
     * 				if (processor ID != 0)
     *	 				MPI_Send (best solution to P0)
     * 			}
     * 		 }
     * 		 else (no message received)
     * 		 {
     * 		    if (local stack empty)
     * 			{
     * 			   choose random CPU asking him for some work // nebo to implementovat jako ACŽ-AHD asynchronní cyklické žádosti před03/sl28
     * 			   blocking MPI_Recv
     * 			   processor received stack
     * 			   continue with the outer 'for' loop
     * 			}
     * 			else (local stack not empty)
     * 			{
     * 			   continue with the outer 'for' loop
     * 			}
     * 		 }
     *    }
     * }     
     * 
     * 
     */

    /* ALGORITMUS */
    while (true) { // nový DFS, field ze stacku nebo z init (dva možné stavy - třeba řešit jen pozice třeba řešit tvar a pozice)
        while (true) { // provedení DFS až do konce
            /*
             * Smyslem kroku je obarvit field jedním konkrétním obdélníkem.
             * Z vlastního předchozího kroku DFS dostanu stav (1) kdy aktuální obdélník fieldu nemá definován ani tvar ani pozici.
             * Ze stacku můžu dostat dva různé stavy (reprezentovány třídou Field) - (2) aktuální obdélník fieldu má jen tvar nebo (3) aktuální obdélník fieldu má tvar i pozici.
             * Takže nad stavem provádím postupně požadované operace (najdu tvary, najdu pozice) dokud není obdélník konkrétní a můžu jím obarvit field.
             * if( (1) ) { řeším tvary }
             * if( (1+tvar) (2) ) { řeším pozice }
             * if( (1+tvar+pozice) (2+pozice) (3) - vždy) { obarvuji }
             */

            /*
             *  Ukončijící podmínka DFS, řešení nalezeno.
             */
            if (field->getRectangles()->getCurrent() == NULL) { // žadný další rect => končím DFS, řešení nalezeno
                if (verbose) cout << "Ending DFS, solution FOUND." << endl;

                /*
                 * Zaznamenání nejlepšího řešení.
                 */
                if (bestField == NULL || field->getPerimetrSum() < bestField->getPerimetrSum()) {
                    delete bestField; // clean-up
                    bestField = field;
                    field = NULL; // protože při načítání nového fieldu ze stacku bych si smazal bestField
                }

                break;
            }

            /*
             * (1)
             */
            if (field->getRectangles()->getCurrent()->hasShape() == false) {
                /*
                 * Řeší tvary aktuálního obdélníku.
                 * První tvar použije pro tento field ostatní pro nové fieldy které vloží na stack.
                 */
                field->solveRectShapes(stack); // vždy existuje alespoň jeden tvar
            }

            /*
             * (1+tvar) (2)
             */
            if (field->getRectangles()->getCurrent()->hasPosition() == false) {
                /*
                 * Řeší pozice aktuálního obdélníku.
                 * První pozici použije pro tento field, ostatní pro nové fieldy které vloží na stack.
                 */
                if (field->solveRectPositions(stack) == false) { // neexistuje žádná možná pozice => končím DFS, řešení nenalezeno
                    if (verbose) cout << "Ending DFS, solution NOT found." << endl;

                    break;
                }
            }

            /* 
             * (1+tvar+pozice) (2+pozice) (3) - vždy
             * Řeší obarvení fieldu aktuálním obdélníkem + posunutí.
             */
            field->colorField();
            field->getRectangles()->toNext();
        }

        /*
         * Načtení dalšího stavu k řešení nového DFS + ukončující podmínka výpočtu.
         */
        delete field;
        field = stack->pop();
        if (field == NULL) {
            // poslat white token pokud ho mam
            // obslouzit zpravy

            /*
             * Test na příchod zprav (muze byt MPI_ANY_TAG)
             * 
             * MSG_WORK_REQUEST - ano obsluhovat - mám prazdnej stack nemám co poslat
             * MSG_WORK_SENT - ano obsluhovat - dostanu nova data a pokracuju vypoctem
             * MSG_WORK_NOWORK ano obsluhovat - nedostanu nic a ptam se dalsiho
             * MSG_TOKEN - ano obsluhovat - řešení ukoncujici podminky 
             * MSG_FINISH - ano obsluhovat - ukoncujici podminka
             * MSG_SOLUTION - nemuze prijit - pouze pri ziskavani konecneho vysledku
             */

            if (verbose || verboseProcessCommunication) cout << myPrefix << "Empty stack." << endl;

            break; // sequential // parallel has to ask other processors
        }

        /*
         * Prvotní rozposláni start fieldů
         */
        if (myID == MASTER && !firstSendExecuted && stack->getSize() >= noIDs - 1) { // jsem Master a jeste jsem poprve nerozesilal a mam dost dat na stacku abych poslal vsem ostatnim
            Field* Fout;
            for (int i = 1; i < noIDs; i++) {
                Fout = stack->popBottom(); // místo tohohle divide stack na n částí kdy jedna tu zůstane ostatní pošlu
                pos = 0;
                Fout->pack(buffer, BUFFER_SIZE, &pos);
                MPI_Send(buffer, BUFFER_SIZE, MPI_PACKED, i, MSG_WORK_SENT, MPI_COMM_WORLD);

                if (verbose || verboseProcessCommunication) cout << myPrefix << "Sent start Field to processor " << i << "." << endl;
                delete Fout;
            }
            firstSendExecuted = true;
        }

        /*
         * Test na příchod pozadavku o praci 
         * 
         * MSG_WORK_REQUEST - ano obsluhuji - řeším jestli mě někdo žádá o práci
         * MSG_WORK_SENT - nemuze prijit - pouze pri prazdnym stacku - nemuze prijit když jsem o práci nežádal
         * MSG_WORK_NOWORK - nemuze prijit - pouze pri prazdnym stacku - nemuze přijit kdyz jsem o praci nezadal
         * MSG_TOKEN - mohl bych obsluhovat ale stačí pouze pri prazdnym stacku - neni třeba odesilat černej kdyz mam ještě data, pockam a odeslu pak bilej, cernej jenom pokud jsem odeslal data procesoru s mensim ID
         * MSG_FINISH - pouze pri prazdnym stacku - nemuze přijit když ještě pracuju
         * MSG_SOLUTION - pouze pri ziskavani konecneho vysledku - nemuze přijit kdyz jeste pracuju prijde az po MSG_FINISH a to jenom Masterovi
         */
    }

    /* waiting for all process, then end */ // na všechny procesy počkám tím že od nich přijmu jejich nej řešení
    // MPI_Barrier(MPI_COMM_WORLD); 

    /* Master */
    if (myID == MASTER) { // Master počkám na zprávy od Slaves a vypíšu řešení
        char isNull;
        for (int i = 1; i < noIDs; i++) {
            pos = 0;
            
            MPI_Recv(buffer, BUFFER_SIZE, MPI_PACKED, MPI_ANY_SOURCE, MSG_SOLUTION, MPI_COMM_WORLD, &status);

            MPI_Unpack(buffer, BUFFER_SIZE, &pos, &isNull, 1, MPI_CHAR, MPI_COMM_WORLD);
            if (isNull == 0) {
                field = Field::unpack(buffer, BUFFER_SIZE, &pos);
                if (bestField == NULL || field->getPerimetrSum() < bestField->getPerimetrSum()) {
                    delete bestField; // clean-up
                    bestField = field;
                }
            }

            if (verbose || verboseProcessCommunication) cout << myPrefix << "Recieved bestField from process " << status.MPI_SOURCE << endl;
        }

        cout << "-------------------- SOLUTION --------------------" << endl;
        if (bestField != NULL) {
            cout << bestField->toString();
        } else {
            cout << "Solution does not exist!" << endl; // muze nastat
        }
        cout << "-------------------- /SOLUTION --------------------" << endl;

        /* time measuring - stop */
        t_end = MPI_Wtime();

        cout << "Calculation took " << (t_end - t_start) << " sec." << endl;
    } else { // Slaves pošlou svoje řešení
        char isNull; // s boolem nejak neslo
        if (bestField != NULL) {
            isNull = 0;
            pos = 0;

            MPI_Pack(&isNull, 1, MPI_CHAR, buffer, BUFFER_SIZE, &pos, MPI_COMM_WORLD);
            bestField->pack(buffer, BUFFER_SIZE, &pos);
        } else { // teoreticky muze nastat
            isNull = 1;

            MPI_Pack(&isNull, 1, MPI_CHAR, buffer, BUFFER_SIZE, &pos, MPI_COMM_WORLD);
        }
        MPI_Send(buffer, BUFFER_SIZE, MPI_PACKED, MASTER, MSG_SOLUTION, MPI_COMM_WORLD);

        if (verbose || verboseProcessCommunication) cout << myPrefix << "Sent my bestField to Master." << endl;
    }


    delete field; // clean-up
    delete bestField; // clean-up
    delete stack; // clean-up

    /* shut down MPI */
    MPI_Finalize();

    exit(EXIT_SUCCESS);
}
