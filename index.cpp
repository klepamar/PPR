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

#include "Field.h"
#include "FieldStack.h"

using namespace std;

/* Global variables */
#define BUFFER_LENGTH 500 // < 1KB aby se posilalo neblokujicim zpusobem
#define MASTER 0
const char *fileName = "input.txt"; // nepsi by bylo pomoci define protoze to potrebuju jen na zacatku
bool verbose = false;
bool verboseStackSize = false;
int myID = -1; // od 0 do noIDs - 1
string myPrefix = "";
int noIDs = -1;

/**
 * Read data from given file and initialize field.
 * @param field
 * @param fileName
 * @return status
 */
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

void processArguments(int argc, char** argv) {
    for (int i = 1; i < argc; i++) { // first argument (argv[0]) is executable name
        if (strcmp(argv[i], "-v") == 0) {
            verbose = true;
        } else if (strcmp(argv[i], "-vs") == 0) {
            verboseStackSize = true;
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
 * ./generator a b n > gen.txt; ./transform.sh a b n gen.txt > trans.txt; ./ppr -f trans.txt
 */
int main(int argc, char** argv) {
    FieldStack* stack = new FieldStack(); // use an implicit constructor to initialise stack pointers & size
    Field* field = NULL;
    Field* bestField = NULL;
    double t_start, t_end;

    /* start up MPI */
    MPI_Init(&argc, &argv);

    /* find out process rank */
    MPI_Comm_rank(MPI_COMM_WORLD, &myID);

    // find prefix for this process
    for (int i = 0; i < myID; i++) {
        myPrefix.append("  ");
    }
    ostringstream tmp;
    tmp << setw(2) << myID;
    myPrefix.append(tmp.str()).append(": ");

    /* find out number of processes */
    MPI_Comm_size(MPI_COMM_WORLD, &noIDs);

    /* cekam na spusteni vsech procesu */ // až pak začnu měřit čas
    MPI_Barrier(MPI_COMM_WORLD);

    if (myID == MASTER) {
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

        cout << "---------- TASK ----------" << endl;
        cout << field->toString();
    }
    
    /* test komunikace */

    /* ALGORITMUS */
    while (false) { // nový DFS, field ze stacku nebo z init (dva možné stavy - třeba řešit jen pozice třeba řešit tvar a pozice)
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
            break; // sequential
            // parallel has to ask other processors
        }
    }

    /* cekam na dokonceni vypoctu */
    MPI_Barrier(MPI_COMM_WORLD);

    if (myID == MASTER) {
        /* time measuring - stop */
        t_end = MPI_Wtime();

        cout << "---------- SOLUTION ----------" << endl;
        if (bestField != NULL) {
            cout << bestField->toString();
        } else {
            cout << "Solution does not exist!" << endl; // muze nastat
        }
        cout << "Calculation took " << (t_end - t_start) << " sec." << endl;
    }

    delete field; // clean-up
    delete bestField; // clean-up
    delete stack; // clean-up

    /* shut down MPI */
    MPI_Finalize();

    exit(EXIT_SUCCESS);
}
