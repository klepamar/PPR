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
#include <time.h>

#include "Field.h"
#include "FieldStack.h"

using namespace std;

/* globals and defines */
#define MASTER 0
#define AM_MASTER (myID == MASTER)

#define SMALL_BUFFER_SIZE 999 // <= 1KB aby se posilalo neblokujicim zpusobem
#define WORK_BUFFER_SIZE 10000

#define WORK_REQUEST_CHECK_FREQUENCY 50
#define SMALLEST_ALLOWED_PROBLEM 2
#define START_FIELDSTACK_SIZE 2 // udelat jinak ten prvotni algoritmus (ve foru dokud muzu tak posilam tolikhle fieldu) - je treba zmenit stack divide abych mohl primo zadat kolik chci odebrat

#define MSG_WORK_REQUEST        1000
#define MSG_WORK_RESPONSE       1001
#define MSG_TOKEN               1003
#define MSG_FINISH              1004
#define MSG_SOLUTION            1005

#define WHITE 1
#define BLACK 0
#define TOKEN_COLOR(color) ((color) ? "white" : "black")


const char *fileName = "input.txt"; // nepsi by bylo pomoci define protoze to potrebuju jen na zacatku, idealni aby normalne cetl ze vstupu a jen prepinacem ze souboru
bool verbose = false; // prepinac -v
bool verboseStackSize = false; // prepinac -vs
bool verboseProcessCommunication = true; // prepinac -vc

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

void waitForLastRequest(MPI_Request* lastRequest, bool &lastRequestValidity) {
    int flag;
    MPI_Status status;

    if (lastRequestValidity == true) { // netestovat pokud request neni validni
        do { // neni mozne prijmout/odeslat praci pokud predchozi neodesla cela
            // if (verbose || verboseProcessCommunication) cout << myPrefix << "Waiting for last request on work buffer to be completed." << endl;
            MPI_Test(lastRequest, &flag, &status);
        } while (!flag);
        lastRequestValidity = false;
    }
}

void sendNoWork(int acceptor) {
    int pos = 0;
    char dummyBuffer[1];
    char isNull = 1;

    if (verbose || verboseProcessCommunication) cout << myPrefix << "Sending work to process " << acceptor << "." << endl;

    MPI_Pack(&isNull, 1, MPI_CHAR, dummyBuffer, 1, &pos, MPI_COMM_WORLD); // no work

    MPI_Send(dummyBuffer, 1, MPI_CHAR, acceptor, MSG_WORK_RESPONSE, MPI_COMM_WORLD);

    if (verbose || verboseProcessCommunication) cout << myPrefix << "Sent NO work to process " << acceptor << "." << endl;
}

/* in/out lastRequestValidity, lastRequestValidity is changed */
void sendWork(char* workBuffer, FieldStack* stackOut, int acceptor, MPI_Request* lastRequest, bool &lastRequestValidity) {
    int pos = 0;
    int flag;
    MPI_Status status;

    /* no work */
    if (stackOut == NULL || stackOut->isEmpty()) {
        sendNoWork(acceptor);
        return;
    }

    /* work */
    if (verbose || verboseProcessCommunication) cout << myPrefix << "Sending work to process " << acceptor << "." << endl;

    waitForLastRequest(lastRequest, lastRequestValidity); // je nutne pockat aby byl workBuffer k pouziti

    char isNull = 0;
    MPI_Pack(&isNull, 1, MPI_CHAR, workBuffer, WORK_BUFFER_SIZE, &pos, MPI_COMM_WORLD); // work
    stackOut->pack(workBuffer, WORK_BUFFER_SIZE, &pos);

    MPI_Isend(workBuffer, WORK_BUFFER_SIZE, MPI_PACKED, acceptor, MSG_WORK_RESPONSE, MPI_COMM_WORLD, lastRequest);

    if (verbose || verboseProcessCommunication) cout << myPrefix << "Sent work to process " << acceptor << "." << endl;

    lastRequestValidity = true;
}

int requestWork() {

    // get donor
    int donor;
    srand(time(NULL));

    do {
        donor = rand() % noIDs;
    } while (donor == myID);

    // send request
    int pos = 0;
    char dummyBuffer[1];

    if (verbose || verboseProcessCommunication) cout << myPrefix << "Requesting work from process " << donor << "." << endl;

    MPI_Send(dummyBuffer, 1, MPI_CHAR, donor, MSG_WORK_REQUEST, MPI_COMM_WORLD);

    if (verbose || verboseProcessCommunication) cout << myPrefix << "Request for work from process " << donor << " sent." << endl;

    return donor;
}

FieldStack* recieveWork(char* workBuffer, int donor, MPI_Request* lastRequest, bool lastRequestValidity) {
    int pos = 0;
    int flag;
    MPI_Status status;

    if (verbose || verboseProcessCommunication) cout << myPrefix << "Waiting for work from process " << donor << "." << endl;

    waitForLastRequest(lastRequest, lastRequestValidity);

    // blokujicim zpusobem cekam na odpoved na zadost o praci, stejne bych nemoh nic delat, jedine odpovidat na zpravy (ale na ty ktery by zpusobily deadlock jsem odpovedel)
    MPI_Recv(workBuffer, WORK_BUFFER_SIZE, MPI_PACKED, donor, MSG_WORK_RESPONSE, MPI_COMM_WORLD, &status);

    char isNull;
    MPI_Unpack(workBuffer, WORK_BUFFER_SIZE, &pos, &isNull, 1, MPI_CHAR, MPI_COMM_WORLD); // jednou tady mam ten dummy ale stejne z nej prectu jen ten byte tak by to nemelo vadit

    if (isNull) { // prace neprisla
        if (verbose || verboseProcessCommunication) cout << myPrefix << "Process " << donor << " sent NO work." << endl;
        return NULL;
    } else { // prace prisla
        FieldStack* FSin = FieldStack::unpack(workBuffer, WORK_BUFFER_SIZE, &pos);
        if (verbose || verboseProcessCommunication) cout << myPrefix << "Process " << donor << " sent work." << endl;
        return FSin;
    }
}

void sendToken(bool tokenColor, bool haveWork, bool &workSentToLower) {
    int nextProcess = (myID + 1) % noIDs;

    if (verbose || verboseProcessCommunication) cout << myPrefix << " * Sending token to process " << nextProcess << "." << endl;

    if (haveWork || workSentToLower) {
        tokenColor = BLACK;
    } // else necham původní

    MPI_Send(&tokenColor, 1, MPI_CHAR, nextProcess, MSG_TOKEN, MPI_COMM_WORLD);
    workSentToLower = false;

    if (verbose || verboseProcessCommunication) cout << myPrefix << " * Sent " << TOKEN_COLOR(tokenColor) << " token to process " << nextProcess << "." << endl;
}

bool recieveToken() {
    bool tokenColor;
    MPI_Status status;

    MPI_Recv(&tokenColor, 1, MPI_CHAR, MPI_ANY_SOURCE, MSG_TOKEN, MPI_COMM_WORLD, &status);

    if (verbose || verboseProcessCommunication) cout << myPrefix << " * Recieved " << TOKEN_COLOR(tokenColor) << " token from process " << status.MPI_SOURCE << "." << endl;

    return tokenColor;
}

/**
 * 
 * $ ./generator a b n > gen.txt; ./transform.sh a b n gen.txt > trans.txt; ./ppr -f trans.txt
 * 
 */
int main(int argc, char** argv) {
    FieldStack* myStack = new FieldStack(); // use an implicit constructor to initialise stack pointers & size
    Field* myCurrField = NULL;
    Field* myBestField = NULL;

    char smallBuffer[SMALL_BUFFER_SIZE];
    char workBuffer[WORK_BUFFER_SIZE];

    double t_start, t_end;
    int comm_pos = 0;
    int comm_flag = 0;
    MPI_Status comm_status;
    MPI_Request comm_request;
    bool comm_request_validity = false;

    bool firstSendExecuted = false; // true after first send from Master to Slaves is executed
    bool tokenColor = false; // true == white (no work), false == black (have work)
    bool haveToken = false;
    bool workSentToLower = false; // true if Pj sent data to Pi where i < j, false after token sent

    bool finishFlag = false;
    int iterationCounter = 0;

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
    if (AM_MASTER) { // master load input data and create first Field
        /* time measuring - start */
        t_start = MPI_Wtime();

        // inicializace
        try {
            processArguments(argc, argv);
            initField(myCurrField, fileName);
        } catch (string ex) {
            cout << "Exception: " << ex << endl;

            delete myCurrField; // clean-up
            delete myBestField; // clean-up
            delete myStack; // clean-up
            exit(EXIT_FAILURE);
        }

        cout << "-------------------- TASK --------------------" << endl;
        cout << "Number of process: " << noIDs << endl;
        cout << myCurrField->toString(true);
        cout << "-------------------- /TASK --------------------" << endl;

    } else { // slaves wait (blocking way) for first data from master
        if (verbose || verboseProcessCommunication) cout << myPrefix << "Waiting for start Field." << endl;

        MPI_Recv(smallBuffer, SMALL_BUFFER_SIZE, MPI_PACKED, MASTER, MSG_WORK_RESPONSE, MPI_COMM_WORLD, &comm_status);
        comm_pos = 0;
        myCurrField = Field::unpack(smallBuffer, SMALL_BUFFER_SIZE, &comm_pos);

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
    while (!finishFlag) { // nový DFS, field ze stacku nebo z init (dva možné stavy - třeba řešit jen pozice třeba řešit tvar a pozice)
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
            if (myCurrField->getRectangles()->getCurrent() == NULL) { // žadný další rect => končím DFS, řešení nalezeno
                if (verbose) cout << "Ending DFS, solution FOUND." << endl;

                /*
                 * Zaznamenání nejlepšího řešení.
                 */
                if (myBestField == NULL || myCurrField->getPerimetrSum() < myBestField->getPerimetrSum()) {
                    delete myBestField; // clean-up
                    myBestField = myCurrField;
                    myCurrField = NULL; // protože při načítání nového fieldu ze stacku bych si smazal bestField
                }

                break;
            }

            /*
             * (1)
             */
            if (myCurrField->getRectangles()->getCurrent()->hasShape() == false) {
                /*
                 * Řeší tvary aktuálního obdélníku.
                 * První tvar použije pro tento field ostatní pro nové fieldy které vloží na stack.
                 */
                myCurrField->solveRectShapes(myStack); // vždy existuje alespoň jeden tvar
            }

            /*
             * (1+tvar) (2)
             */
            if (myCurrField->getRectangles()->getCurrent()->hasPosition() == false) {
                /*
                 * Řeší pozice aktuálního obdélníku.
                 * První pozici použije pro tento field, ostatní pro nové fieldy které vloží na stack.
                 */
                if (myCurrField->solveRectPositions(myStack) == false) { // neexistuje žádná možná pozice => končím DFS, řešení nenalezeno
                    if (verbose) cout << "Ending DFS, solution NOT found." << endl;

                    break;
                }
            }

            /* 
             * (1+tvar+pozice) (2+pozice) (3) - vždy
             * Řeší obarvení fieldu aktuálním obdélníkem + posunutí.
             */
            myCurrField->colorField();
            myCurrField->getRectangles()->toNext();
        }

        /*
         * Načtení dalšího stavu k řešení nového DFS + ukončující podmínka výpočtu.
         */
        delete myCurrField;
        myCurrField = myStack->pop();

        while (myCurrField == NULL) { // stack is empty

            if (noIDs == 1) { // is not parallel
                finishFlag = true; // ukoncujici podminka vnejsiho whilu (celeho algoritmu)
                break; // vyskoci z totohle whilu kterej zada o praci // sekvencne nema koho pozadat
            }

            // 1) poslat white token pokud ho mam a prisel white
            // 2) pozadat o data
            // 3) obslouzit zpravy


            // 1) TOKEN
            if (AM_MASTER) { // pokud ma Master token obarvi ho na bilo a posle
                if (haveToken) {
                    tokenColor = WHITE;
                    sendToken(tokenColor, false, workSentToLower);
                    haveToken = false;
                }
            } else {
                if (haveToken) {
                    sendToken(tokenColor, false, workSentToLower);
                    haveToken = false;
                }
            }


            // 2) DONOR REQUEST
            int donor = requestWork();


            // 3) HANDLE MESSAGES
            /*
             * MSG_WORK_REQUEST - ano obsluhovat - mám prazdnej stack nemám co poslat
             * MSG_WORK_RESPONSE - neobsluhovat - pockam si na ni az samostatne, urcite musi odpovedet nedojde k deadlocku
             * MSG_TOKEN - ano obsluhovat - řešení ukoncujici podminky 
             * MSG_FINISH - ano obsluhovat - ukoncujici podminka
             * MSG_SOLUTION - nemuze prijit - pouze pri ziskavani konecneho vysledku
             */

            if (verbose || verboseProcessCommunication) cout << myPrefix << "Handle messages." << endl;

            bool anyMessage = false;
            do { // mohlo prijit vic pozadavku

                MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &comm_flag, &comm_status);

                if (comm_flag) { // prisla zprava

                    anyMessage = true;

                    switch (comm_status.MPI_TAG) {
                        case MSG_WORK_REQUEST: // nemam dostatek prace
                            if (verbose || verboseProcessCommunication) cout << myPrefix << "Process " << comm_status.MPI_SOURCE << " request work." << endl;
                            sendNoWork(comm_status.MPI_SOURCE);
                            break;

                        case MSG_WORK_RESPONSE: // prisla mi zpatky odpoved (nemusi v ni bejt ale prace)
                            delete myStack;
                            myStack = recieveWork(workBuffer, donor, &comm_request, comm_request_validity);
                            break;

                        case MSG_TOKEN:
                            tokenColor = recieveToken();
                            haveToken = true;
                            break;

                        case MSG_FINISH:
                            // comm_flag = 0; // ani nedoresi vsechny zpravy // prijde mi nebezpecne
                            finishFlag = true; // ukoncujici podminka vnejsiho whilu (celeho algoritmu)
                            break;

                        default:
                            if (verbose || verboseProcessCommunication) cout << myPrefix << "Not known or not expected tag " << comm_status.MPI_TAG << " from process " << comm_status.MPI_SOURCE << "." << endl;
                            break;
                    }
                }
            } while (comm_flag);

            if (anyMessage == false) {
                if (verbose || verboseProcessCommunication) cout << myPrefix << "NO messages." << endl;
            }

            if (finishFlag) { // ukoncující podmínka // uz nepockam ani na zpravu o praci - vim ze bych zadnou praci nedostal
                break;
            }

            // pokud stale nemam praci (zatim jsem nedostal odpoved nebo jsem dostal odpoved ve ktery prace nebyla) tak blokujicne cekam
            if (myStack == NULL || myStack->isEmpty()) {
                delete myStack;
                myStack = recieveWork(workBuffer, donor, &comm_request, comm_request_validity);
            }

            if (myStack != NULL) { // asi by null bejt nikdy neměl ale projistotu
                myCurrField = myStack->pop(); // tim zajistím vyskočení z cyklu
            }
            // else budu cyklus opakovat (zazadam noveho, odpovim na zpravy a cekam na praci)
        }

        /*
         * Prvotní rozposláni start fieldů a tokenu
         */
        if (AM_MASTER && !firstSendExecuted && myStack->getSize() >= noIDs - 1) { // jsem Master a jeste jsem poprve nerozesilal a mam dost dat na stacku abych poslal vsem ostatnim
            Field* Fout;
            for (int i = 1; i < noIDs; i++) {
                if (verbose || verboseProcessCommunication) cout << myPrefix << "Sending start Field to process " << i << "." << endl;

                Fout = myStack->popBottom(); // místo tohohle divide stack na n částí kdy jedna tu zůstane ostatní pošlu
                comm_pos = 0;
                Fout->pack(smallBuffer, SMALL_BUFFER_SIZE, &comm_pos);
                MPI_Send(smallBuffer, SMALL_BUFFER_SIZE, MPI_PACKED, i, MSG_WORK_RESPONSE, MPI_COMM_WORLD);

                if (verbose || verboseProcessCommunication) cout << myPrefix << "Sent start Field to process " << i << "." << endl;

                delete Fout;
            }
            firstSendExecuted = true;

            // prvni token
            sendToken(WHITE, !myStack->isEmpty(), workSentToLower); // pradepodobne se posle cernej protoze mam neco na stacku
            haveToken = false;
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
        iterationCounter++;
        if (iterationCounter == WORK_REQUEST_CHECK_FREQUENCY) {

            if (verbose || verboseProcessCommunication) cout << myPrefix << "Handle work requests." << endl;

            bool anyRequest = false;
            do { // mohlo prijit vic pozadavku
                MPI_Iprobe(MPI_ANY_SOURCE, MSG_WORK_REQUEST, MPI_COMM_WORLD, &comm_flag, &comm_status);
                if (comm_flag) { // prisel pozadavek na praci
                    anyRequest = true;

                    if (verbose || verboseProcessCommunication) cout << myPrefix << "Process " << comm_status.MPI_SOURCE << " request work." << endl;

                    FieldStack* FSout = myStack->divide();
                    sendWork(workBuffer, FSout, comm_status.MPI_SOURCE, &comm_request, comm_request_validity);

                    delete FSout;
                }
            } while (comm_flag);

            if (anyRequest == false) {
                if (verbose || verboseProcessCommunication) cout << myPrefix << "NO work requests." << endl;
            }

            iterationCounter = 0;
        }
    }

    cout << myPrefix << "Venku z algoritmu." << endl; // sem se nedostanu

    /* waiting for all process, then end */ // na všechny procesy počkám tím že od nich přijmu jejich nej řešení
    // MPI_Barrier(MPI_COMM_WORLD); 

    /* Master */
    if (AM_MASTER) { // Master počkám na zprávy od Slaves a vypíšu řešení
        char isNull;

        if (verbose || verboseProcessCommunication) cout << myPrefix << "Gathering best Fields from other processes." << endl;

        for (int i = 1; i < noIDs; i++) {
            comm_pos = 0;

            MPI_Recv(smallBuffer, SMALL_BUFFER_SIZE, MPI_PACKED, MPI_ANY_SOURCE, MSG_SOLUTION, MPI_COMM_WORLD, &comm_status);

            MPI_Unpack(smallBuffer, SMALL_BUFFER_SIZE, &comm_pos, &isNull, 1, MPI_CHAR, MPI_COMM_WORLD);
            if (isNull == 0) {
                myCurrField = Field::unpack(smallBuffer, SMALL_BUFFER_SIZE, &comm_pos);
                if (myBestField == NULL || myCurrField->getPerimetrSum() < myBestField->getPerimetrSum()) {
                    delete myBestField; // clean-up
                    myBestField = myCurrField;
                }
            }

            if (verbose || verboseProcessCommunication) cout << myPrefix << "Received best Field from process " << comm_status.MPI_SOURCE << "." << endl;
        }

        cout << "-------------------- SOLUTION --------------------" << endl;
        if (myBestField != NULL) {
            cout << myBestField->toString(true);
        } else {
            cout << "Solution does not exist!" << endl; // muze nastat
        }
        /* time measuring - stop */
        t_end = MPI_Wtime();

        cout << "Calculation took " << (t_end - t_start) << " sec." << endl;
        cout << "-------------------- /SOLUTION --------------------" << endl;



    } else { // Slaves pošlou svoje řešení
        if (verbose || verboseProcessCommunication) cout << myPrefix << "Sending my best Field to Master." << endl;

        char isNull; // s boolem nejak neslo
        if (myBestField != NULL) {
            isNull = 0;
            comm_pos = 0;

            MPI_Pack(&isNull, 1, MPI_CHAR, smallBuffer, SMALL_BUFFER_SIZE, &comm_pos, MPI_COMM_WORLD);
            myBestField->pack(smallBuffer, SMALL_BUFFER_SIZE, &comm_pos);
        } else { // teoreticky muze nastat
            isNull = 1;
            comm_pos = 0;

            MPI_Pack(&isNull, 1, MPI_CHAR, smallBuffer, SMALL_BUFFER_SIZE, &comm_pos, MPI_COMM_WORLD);
        }
        MPI_Send(smallBuffer, SMALL_BUFFER_SIZE, MPI_PACKED, MASTER, MSG_SOLUTION, MPI_COMM_WORLD);

        if (verbose || verboseProcessCommunication) cout << myPrefix << "Sent my best Field to Master." << endl;
    }


    delete myCurrField; // clean-up
    delete myBestField; // clean-up
    delete myStack; // clean-up

    /* shut down MPI */
    MPI_Finalize();

    exit(EXIT_SUCCESS);
}
