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
#include <unistd.h>

#include "Field.h"
#include "FieldStack.h"

using namespace std;

/* globals and defines */
#define WORK_REQUEST_CHECK_FREQUENCY 50
#define SMALLEST_ALLOWED_PROBLEM 2
#define START_FIELDSTACK_SIZE 2 // udelat jinak ten prvotni algoritmus (ve foru dokud muzu tak posilam tolikhle fieldu) - je treba zmenit stack divide abych mohl primo zadat kolik chci odebrat
#define SLEEP_TIME 50 * 1000 // microseconds

#define MASTER 0
#define AM_MASTER (myID == MASTER)

#define ZERO_BUFFER_SIZE        0
#define TINY_BUFFER_SIZE        1 // for empty messages // nevim jeslti to ma smysl snazit se posilat min, jestli se stejne vzdycky neposle ten 1KB paket
#define WORK_BUFFER_SIZE        8 * 1000000 // k * 1MB

char tinyBuffer[TINY_BUFFER_SIZE];
char workBuffer[WORK_BUFFER_SIZE];

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
bool verboseProcessCommunication = false; // prepinac -vc

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

void waitForLastWorkRequest(MPI_Request* lastRequest, bool &lastRequestValidity) {
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

int getDonor() {
    int donor;
    srand(time(NULL));

    do {
        donor = rand() % noIDs;
    } while (donor == myID);
}

void sendWorkRequest(int donor) {
    if (verbose || verboseProcessCommunication) cout << myPrefix << "Sendding work request to process " << donor << "." << endl;

    MPI_Send(tinyBuffer, ZERO_BUFFER_SIZE, MPI_CHAR, donor, MSG_WORK_REQUEST, MPI_COMM_WORLD);

    if (verbose || verboseProcessCommunication) cout << myPrefix << "Sent work request to process " << donor << "." << endl;
}

void receiveWorkRequest(int acceptor) {
    MPI_Status status;

    if (verbose || verboseProcessCommunication) cout << myPrefix << "Receiving work request from process " << acceptor << "." << endl;

    MPI_Recv(tinyBuffer, ZERO_BUFFER_SIZE, MPI_CHAR, acceptor, MSG_WORK_REQUEST, MPI_COMM_WORLD, &status);

    if (verbose || verboseProcessCommunication) cout << myPrefix << "Received work request from process " << status.MPI_SOURCE << "." << endl;
}

void sendNoWorkResponse(int acceptor) {
    int pos = 0;
    char isNull = 1; // mean I sent no work

    if (verbose || verboseProcessCommunication) cout << myPrefix << "Sending work to process " << acceptor << "." << endl;

    MPI_Pack(&isNull, 1, MPI_CHAR, tinyBuffer, TINY_BUFFER_SIZE, &pos, MPI_COMM_WORLD);
    MPI_Send(tinyBuffer, TINY_BUFFER_SIZE, MPI_CHAR, acceptor, MSG_WORK_RESPONSE, MPI_COMM_WORLD);

    if (verbose || verboseProcessCommunication) cout << myPrefix << "Sent NO work to process " << acceptor << "." << endl;
}

/* in/out lastRequestValidity, lastRequestValidity is changed */
void sendWorkResponse(FieldStack* stackOut, int acceptor, MPI_Request* lastRequest, bool &lastRequestValidity) {
    int pos = 0;
    MPI_Status status;

    /* no work */
    if (stackOut == NULL || stackOut->isEmpty()) {
        sendNoWorkResponse(acceptor);
        return;
    }

    /* work */
    if (verbose || verboseProcessCommunication) cout << myPrefix << "Sending work to process " << acceptor << "." << endl;

    waitForLastWorkRequest(lastRequest, lastRequestValidity); // je nutne pockat aby byl workBuffer k pouziti

    char isNull = 0; // mean I sent work
    MPI_Pack(&isNull, 1, MPI_CHAR, workBuffer, WORK_BUFFER_SIZE, &pos, MPI_COMM_WORLD);
    stackOut->pack(workBuffer, WORK_BUFFER_SIZE, &pos);

    MPI_Isend(workBuffer, WORK_BUFFER_SIZE, MPI_PACKED, acceptor, MSG_WORK_RESPONSE, MPI_COMM_WORLD, lastRequest);
    lastRequestValidity = true;

    if (verbose || verboseProcessCommunication) cout << myPrefix << "Sent SOME work to process " << acceptor << "." << endl;
}

/* in/out lastRequestValidity, lastRequestValidity is changed */
FieldStack* receiveWorkResponse(int donor, MPI_Request* lastRequest, bool &lastRequestValidity) {
    int pos = 0;
    MPI_Status status;

    if (verbose || verboseProcessCommunication) cout << myPrefix << "Receiving work from process " << donor << "." << endl;

    waitForLastWorkRequest(lastRequest, lastRequestValidity);

    // blokujicim zpusobem cekam na odpoved na zadost o praci, stejne bych nemoh nic delat, jedine odpovidat na zpravy (ale na ty ktery by zpusobily deadlock jsem odpovedel)
    MPI_Recv(workBuffer, WORK_BUFFER_SIZE, MPI_PACKED, donor, MSG_WORK_RESPONSE, MPI_COMM_WORLD, &status);

    char isNull;
    MPI_Unpack(workBuffer, WORK_BUFFER_SIZE, &pos, &isNull, 1, MPI_CHAR, MPI_COMM_WORLD); // jednou tady mam ten dummy ale stejne z nej prectu jen ten byte tak by to nemelo vadit

    if (isNull) { // prace neprisla
        if (verbose || verboseProcessCommunication) cout << myPrefix << "Received NO work from process " << status.MPI_SOURCE << endl;
        return NULL;
    } else { // prace prisla
        FieldStack* FSin = FieldStack::unpack(workBuffer, WORK_BUFFER_SIZE, &pos);
        if (verbose || verboseProcessCommunication) cout << myPrefix << "Received SOME work from process " << status.MPI_SOURCE << endl;
        return FSin;
    }
}

void sendToken(char tokenColor, bool haveWork, bool &workSentToLower) { // prijemce je pevne dany
    int nextProcess = (myID + 1) % noIDs;

    if (verbose || verboseProcessCommunication) cout << myPrefix << "Sending token to process " << nextProcess << "." << endl;

    if (haveWork || workSentToLower) {
        tokenColor = BLACK;
    } // else necham původní

    MPI_Send(&tokenColor, 1, MPI_CHAR, nextProcess, MSG_TOKEN, MPI_COMM_WORLD);
    workSentToLower = false; // vynulovat

    if (verbose || verboseProcessCommunication) cout << myPrefix << "Sent " << TOKEN_COLOR(tokenColor) << " token to process " << nextProcess << "." << endl;
}

bool receiveToken() { // odesilatel je pevne dany ale pro zjednoduseni prijimam ode vsech
    char tokenColor;
    MPI_Status status;

    if (verbose || verboseProcessCommunication) cout << myPrefix << "Receiving token." << endl;

    MPI_Recv(&tokenColor, 1, MPI_CHAR, MPI_ANY_SOURCE, MSG_TOKEN, MPI_COMM_WORLD, &status);

    if (verbose || verboseProcessCommunication) cout << myPrefix << "Received " << TOKEN_COLOR(tokenColor) << " token from process " << status.MPI_SOURCE << "." << endl;

    return (bool)tokenColor;
}

void sendFinish(int acceptor) {
    if (verbose || verboseProcessCommunication) cout << myPrefix << "Sending finish message to process " << acceptor << "." << endl;

    MPI_Send(tinyBuffer, ZERO_BUFFER_SIZE, MPI_CHAR, acceptor, MSG_FINISH, MPI_COMM_WORLD);

    if (verbose || verboseProcessCommunication) cout << myPrefix << "Sent finish message to process " << acceptor << "." << endl;
}

void receiveFinish() { // odesilatel jen Master ale nechavam projistotu ANY
    MPI_Status status;

    if (verbose || verboseProcessCommunication) cout << myPrefix << "Receiving finish message." << endl;

    MPI_Recv(tinyBuffer, ZERO_BUFFER_SIZE, MPI_CHAR, MPI_ANY_SOURCE, MSG_FINISH, MPI_COMM_WORLD, &status);

    if (verbose || verboseProcessCommunication) cout << myPrefix << "Received finish message from process " << status.MPI_SOURCE << "." << endl;
}

void sendSolution(Field* bestSolution, MPI_Request* lastRequest, bool &lastRequestValidity) { // prijemce je vzdy Master
    int pos = 0;
    
    if (verbose || verboseProcessCommunication) cout << myPrefix << "Sending best Field to Master." << endl;
    
    waitForLastWorkRequest(lastRequest, lastRequestValidity);

    char isNull;
    if (bestSolution != NULL) {
        isNull = 0;

        MPI_Pack(&isNull, 1, MPI_CHAR, workBuffer, WORK_BUFFER_SIZE, &pos, MPI_COMM_WORLD);
        bestSolution->pack(workBuffer, WORK_BUFFER_SIZE, &pos);
        MPI_Send(workBuffer, WORK_BUFFER_SIZE, MPI_PACKED, MASTER, MSG_SOLUTION, MPI_COMM_WORLD);
        lastRequestValidity = true;
        
        if (verbose || verboseProcessCommunication) cout << myPrefix << "Sent NULL best Field to Master." << endl;
    } else { // nema zadne reseni
        isNull = 1;

        MPI_Pack(&isNull, 1, MPI_CHAR, tinyBuffer, TINY_BUFFER_SIZE, &pos, MPI_COMM_WORLD);
        MPI_Send(tinyBuffer, TINY_BUFFER_SIZE, MPI_PACKED, MASTER, MSG_SOLUTION, MPI_COMM_WORLD);
        
        if (verbose || verboseProcessCommunication) cout << myPrefix << "Sent NULL best Field to Master." << endl;
    }
}

/* in/out lastRequestValidity, lastRequestValidity is changed */
Field* receiveSolution(MPI_Request* lastRequest, bool &lastRequestValidity) { // odesilatel muze bejt kdokoliv
    Field* field;
    char isNull;
    int pos = 0;
    MPI_Status status;

    if (verbose || verboseProcessCommunication) cout << myPrefix << "Receiving best Field." << endl;

    waitForLastWorkRequest(lastRequest, lastRequestValidity);

    MPI_Recv(workBuffer, WORK_BUFFER_SIZE, MPI_PACKED, MPI_ANY_SOURCE, MSG_SOLUTION, MPI_COMM_WORLD, &status);

    MPI_Unpack(workBuffer, WORK_BUFFER_SIZE, &pos, &isNull, 1, MPI_CHAR, MPI_COMM_WORLD);
    if (isNull == 0) {
        field = Field::unpack(workBuffer, WORK_BUFFER_SIZE, &pos);
        if (verbose || verboseProcessCommunication) cout << myPrefix << "Received SOME best Field from process " << status.MPI_SOURCE << "." << endl;
    } else {
        field = NULL;
        if (verbose || verboseProcessCommunication) cout << myPrefix << "Received NULL best Field from process " << status.MPI_SOURCE << "." << endl;
    }

    return field;
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

    double t_start, t_end;
    int comm_pos = 0;
    int comm_flag = 0;
    MPI_Status comm_status;
    MPI_Request comm_request;
    bool comm_requestValidity = false;

    bool firstSendExecuted = false; // true after first send from Master to Slaves is executed
    bool tokenColor = false; // true == white (no work), false == black (have work)
    bool haveToken = false;
    bool workSentToLower = false; // true if Pj sent data to Pi where i < j, false after token sent

    bool finishFlag = false;
    int iterationCounter = 0;

    int gatheredSolutins = 0;
    int divideStackCalled = 0;

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

        // neni treba cekat je to poprve co poouziju workBuffer
        /* TODO az bude minimalDivide tak pouzit normalne receive work */
        MPI_Recv(workBuffer, WORK_BUFFER_SIZE, MPI_PACKED, MASTER, MSG_WORK_RESPONSE, MPI_COMM_WORLD, &comm_status);
        comm_pos = 0;
        myCurrField = Field::unpack(workBuffer, WORK_BUFFER_SIZE, &comm_pos);

        if (verbose || verboseProcessCommunication) cout << myPrefix << "Received start Field." << endl;
    }


    /* 
     * ALGORITMUS
     */
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
                improveSolution(myBestField, myCurrField);
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

            // 1) vyresit token - pripadne ukonceni vypoctu
            // 2) pozadat o praci - aby mohl pokracovat
            // 3) obsluhovat prichazejici zpravy - dokola aby nedoslo k deadlocku


            // 1) TOKEN
            if (AM_MASTER) { // pokud ma Master token obarvi ho na bilo a posle
                if (haveToken) {
                    if (tokenColor == WHITE) { // vratil se mi bilej takze nikdo nema praci, ukonci

                        for (int i = 1; i < noIDs; i++) { // send finish message to all other process
                            sendFinish(i);
                        }

                        finishFlag = true; // finish 
                        break; // teď už je to v pohodě nikde necekam blokujicne
                    } else { // obarvi na bilo a posli znovu
                        tokenColor = WHITE;
                        sendToken(tokenColor, false, workSentToLower);
                        haveToken = false;
                    }
                }
            } else { // Slaves pripadne obarvi preposlou
                if (haveToken) {
                    sendToken(tokenColor, false, workSentToLower);
                    haveToken = false;
                }
            }


            // 2) DONOR REQUEST
            int donor;
            if (!finishFlag) { // nezadat praci pokud koncim ale musim vyresit dotazy
                donor = getDonor();
                sendWorkRequest(donor);
            }


            // 3) HANDLE MESSAGES
            bool receivedResponse;
            while (true) { // abych pockal na work response

                if (verbose || verboseProcessCommunication) cout << myPrefix << "Handle messages." << endl;

                receivedResponse = false;
                do { // mohlo prijit vic pozadavku

                    MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &comm_flag, &comm_status);

                    if (comm_flag) { // prisla zprava

                        switch (comm_status.MPI_TAG) {
                            case MSG_WORK_REQUEST:
                                receiveWorkRequest(comm_status.MPI_SOURCE);
                                sendNoWorkResponse(comm_status.MPI_SOURCE); // nemam dostatek prace
                                break;

                            case MSG_WORK_RESPONSE: // prisla mi zpatky odpoved (nemusi v ni bejt ale prace)
                                receivedResponse = true;
                                delete myStack;
                                myStack = receiveWorkResponse(donor, &comm_request, comm_requestValidity);
                                break;

                            case MSG_TOKEN:
                                tokenColor = receiveToken();
                                haveToken = true;
                                break;

                            case MSG_FINISH:
                                receiveFinish();

                                comm_flag = 0; // uz by mozna nemusel doresovat ani ostatni zpravy protoze kdyz MSG_FINISH dostane jeden dostanou ji vsichni
                                finishFlag = true; // ukoncujici podminka celeho algoritmu
                                break;

                            case MSG_SOLUTION:
                                gatheredSolutins++;
                                myCurrField = receiveSolution(&comm_request, comm_requestValidity);
                                improveSolution(myBestField, myCurrField);
                                break;

                            default:
                                if (verbose || verboseProcessCommunication) cout << myPrefix << "Not known or not expected tag " << comm_status.MPI_TAG << " from process " << comm_status.MPI_SOURCE << "." << endl;
                                break;
                        }
                    }
                } while (comm_flag);

                if (!receivedResponse && !finishFlag) { // jeste jsem nedostal odpoved na zadost o praci -> je treba pockat na odpoved
                    if (verbose || verboseProcessCommunication) cout << myPrefix << "No more messages, but still do not have work response. Sleep..." << endl;
                    usleep(SLEEP_TIME);
                    continue;
                } else {
                    break;
                }
            }

            if (finishFlag) { // ukoncující podmínka // uz nepockam ani na zpravu o praci - vim ze bych zadnou praci nedostal
                break;
            }

            if (myStack != NULL) { // asi by null bejt nikdy neměl ale projistotu
                myCurrField = myStack->pop(); // pokud jsem dostal praci zajistím tim vyskočení z cyklu
            }
            // else budu cyklus opakovat (zazadam noveho, odpovim na zpravy a cekam na praci)
        }

        if (finishFlag) { // neni treba uz pokracovat
            break;
        }

        /*
         * Prvotní rozposláni start fieldů a tokenu
         */
        if (AM_MASTER && !firstSendExecuted && myStack->getSize() >= noIDs - 1) { // jsem Master a jeste jsem poprve nerozesilal a mam dost dat na stacku abych poslal vsem ostatnim
            Field* Fout;
            for (int i = 1; i < noIDs; i++) {
                if (verbose || verboseProcessCommunication) cout << myPrefix << "Sending start Field to process " << i << "." << endl;

                waitForLastWorkRequest(&comm_request, comm_requestValidity);
                /* TODO az bude minimal divide pouzit normalne receive work, slo by i tou redukci*/
                Fout = myStack->popBottom(); // místo tohohle divide stack na n částí kdy jedna tu zůstane ostatní pošlu
                comm_pos = 0;
                Fout->pack(workBuffer, WORK_BUFFER_SIZE, &comm_pos);
                //MPI_Send(workBuffer, WORK_BUFFER_SIZE, MPI_PACKED, i, MSG_WORK_RESPONSE, MPI_COMM_WORLD);
                MPI_Isend(workBuffer, WORK_BUFFER_SIZE, MPI_PACKED, i, MSG_WORK_RESPONSE, MPI_COMM_WORLD, &comm_request);
                comm_requestValidity = true;

                if (verbose || verboseProcessCommunication) cout << myPrefix << "Sent start Field to process " << i << "." << endl;

                delete Fout;
            }
            firstSendExecuted = true;

            // prvni token
            sendToken(WHITE, !myStack->isEmpty(), workSentToLower); // pradepodobne se posle cernej protoze mam neco na stacku
            haveToken = false;
        }

        /*
         * Test jen na zadost o praci, ostatni resim pri prazdnym stacku
         */
        iterationCounter++;
        if (iterationCounter == WORK_REQUEST_CHECK_FREQUENCY) {

            if (verbose || verboseProcessCommunication) cout << myPrefix << "Handle work requests." << endl;

            bool anyRequest = false;
            do { // mohlo prijit vic pozadavku
                MPI_Iprobe(MPI_ANY_SOURCE, MSG_WORK_REQUEST, MPI_COMM_WORLD, &comm_flag, &comm_status);
                if (comm_flag) { // prisel pozadavek na praci
                    anyRequest = true;

                    receiveWorkRequest(comm_status.MPI_SOURCE);

                    if (verbose || verboseProcessCommunication) divideStackCalled++;
                    FieldStack* FSout = myStack->divide();
                    sendWorkResponse(FSout, comm_status.MPI_SOURCE, &comm_request, comm_requestValidity);

                    delete FSout;
                }
            } while (comm_flag);

            if (anyRequest == false) {
                if (verbose || verboseProcessCommunication) cout << myPrefix << "NO work requests." << endl;
            }

            iterationCounter = 0;
        }
    }

    /* waiting for all process, then end */ // na všechny procesy počkám tím že od nich přijmu jejich nej řešení
    // MPI_Barrier(MPI_COMM_WORLD); 

    /* Master */
    if (AM_MASTER) { // Master počkám na zprávy od Slaves a vypíšu řešení
        if (verbose || verboseProcessCommunication) cout << myPrefix << "Gathering rest of best Fields from other processes." << endl;

        for (int i = 1; i < noIDs - gatheredSolutins; i++) { // pockam jeste na zbytek reseni (ne ty ktery neprisli kdyz jsem obsluhoval posledni zpravy)
            myCurrField = receiveSolution(&comm_request, comm_requestValidity);
            improveSolution(myBestField, myCurrField);
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
        sendSolution(myBestField, &comm_request, comm_requestValidity);
    }


    delete myCurrField; // clean-up
    delete myBestField; // clean-up
    delete myStack; // clean-up

    /* shut down MPI */

    // each CPU displays how many times stack was divided
    sleep(1);
    if (verbose || verboseProcessCommunication) cout << myPrefix << "Divided stack: " << divideStackCalled << " times." << endl;

    MPI_Finalize();

    exit(EXIT_SUCCESS);
}
