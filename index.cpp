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
// tyhle je potreba otestovat a nastavit na co nejlepsi hodnoty - muzou ovlivnovat dobu behu
#define WORK_REQUEST_CHECK_FREQUENCY 50
#define SMALLEST_ALLOWED_PROBLEM 2
#define START_FIELDSTACK_SIZE 2 // udelat jinak ten prvotni algoritmus (ve foru dokud muzu tak posilam tolikhle fieldu) - je treba zmenit stack divide abych mohl primo zadat kolik chci odebrat
#define SLEEP_TIME 50 * 1000 // k * 1ms

#define MASTER 0
#define AM_MASTER (myID == MASTER)

#define ZERO_BUFFER_SIZE        0 // for empty messages
#define TINY_BUFFER_SIZE        100 // nevim jeslti to ma smysl snazit se posilat min, jestli se stejne vzdycky neposle ten 1KB paket
#define WORK_BUFFER_SIZE        100 * 1000 // 1kB = 1000, 1MB = 1 000 000B

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

const char* filename = "input.txt";
bool verbose = false; // prepinac -v
bool verboseStackSize = false; // prepinac -vs
bool verboseProcessCommunication = false; // prepinac -vc

int myID = -1; // od 0 do noIDs-1
string myPrefix = "";
int noIDs = -1;

/* Read data from given file and initialize given field. */
void initField(Field* &field) {
    int a, b; // a, b - dimension of field
    int n; // number of non-zero numbers
    string dummy;
    ifstream in;

    // open file
    in.open(filename);
    if (!in.is_open()) {
        ostringstream ss;
        ss << "Could not open a file \"" << filename << "\"!";
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
    // cout << myPrefix << "processing " << argc << " arguments." << endl;

    for (int i = 1; i < argc; i++) { // first argument (argv[0]) is executable name
        if (strcmp(argv[i], "-v") == 0) {
            verbose = true;
        } else if (strcmp(argv[i], "-vs") == 0) {
            verboseStackSize = true;
        } else if (strcmp(argv[i], "-vc") == 0) {
            verboseProcessCommunication = true;
        } else if (strcmp(argv[i], "-f") == 0) {
            filename = argv[i + 1];
            i++; // skip next, already resolved
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

    return donor;
}

void sendWorkRequest(int donor) {
    if (verbose || verboseProcessCommunication) cout << myPrefix << "Sendding work request to process " << donor << "." << endl;

    MPI_Send(tinyBuffer, ZERO_BUFFER_SIZE, MPI_CHAR, donor, MSG_WORK_REQUEST, MPI_COMM_WORLD);

    if (verbose || verboseProcessCommunication) cout << myPrefix << "Sent work request to process " << donor << "." << endl;
}

void receiveWorkRequest(int acceptor) {
    MPI_Status status;

    if (verbose || verboseProcessCommunication) cout << myPrefix << "Receiving work request." << endl;

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
void sendWorkResponse(FieldStack* stackOut, Field* bestSolution, int acceptor, MPI_Request* lastRequest, bool &lastRequestValidity) {
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
    MPI_Pack(&isNull, 1, MPI_CHAR, workBuffer, WORK_BUFFER_SIZE, &pos, MPI_COMM_WORLD); // zapackuju ze posilam praci
    if (bestSolution == NULL) {
        isNull = 1;
        MPI_Pack(&isNull, 1, MPI_CHAR, workBuffer, WORK_BUFFER_SIZE, &pos, MPI_COMM_WORLD); // zapackuju ze neposilam best solution
    } else {
        isNull = 0;
        MPI_Pack(&isNull, 1, MPI_CHAR, workBuffer, WORK_BUFFER_SIZE, &pos, MPI_COMM_WORLD); // zapackuju ze posilam best solution
        bestSolution->pack(workBuffer, WORK_BUFFER_SIZE, &pos); // zapackuju best solution
    }
    stackOut->pack(workBuffer, WORK_BUFFER_SIZE, &pos); // zapackuju zbytek prace

    MPI_Isend(workBuffer, WORK_BUFFER_SIZE, MPI_PACKED, acceptor, MSG_WORK_RESPONSE, MPI_COMM_WORLD, lastRequest);
    lastRequestValidity = true;

    if (verbose || verboseProcessCommunication) cout << myPrefix << "Sent SOME work to process " << acceptor << "." << endl;
}

/* in/out lastRequestValidity, lastRequestValidity is changed */
FieldStack* receiveWorkResponse(Field* &bestSolution, int donor, MPI_Request* lastRequest, bool &lastRequestValidity) {
    int pos = 0;
    MPI_Status status;

    if (verbose || verboseProcessCommunication) cout << myPrefix << "Receiving work from process " << donor << "." << endl;

    waitForLastWorkRequest(lastRequest, lastRequestValidity);

    // blokujicim zpusobem cekam na odpoved na zadost o praci, ale volal jsem tu funkci ve chvili kdy vim ze mi to prislo
    MPI_Recv(workBuffer, WORK_BUFFER_SIZE, MPI_PACKED, donor, MSG_WORK_RESPONSE, MPI_COMM_WORLD, &status);

    char isNull;
    MPI_Unpack(workBuffer, WORK_BUFFER_SIZE, &pos, &isNull, 1, MPI_CHAR, MPI_COMM_WORLD); // jednou tady mam ten dummy ale stejne z nej prectu jen ten byte tak by to nemelo vadit

    if (isNull) { // prace neprisla
        if (verbose || verboseProcessCommunication) cout << myPrefix << "Received NO work from process " << status.MPI_SOURCE << endl;
        return NULL;
    } else { // prace prisla
        MPI_Unpack(workBuffer, WORK_BUFFER_SIZE, &pos, &isNull, 1, MPI_CHAR, MPI_COMM_WORLD);
        if (!isNull) { // prislo i best solution
            Field* BSin = Field::unpack(workBuffer, WORK_BUFFER_SIZE, &pos);
            improveSolution(bestSolution, BSin); // zlepsim si svoje nej reseni podle nej reseni toho kdo mi poslal praci
        }

        FieldStack* FSin = FieldStack::unpack(workBuffer, WORK_BUFFER_SIZE, &pos); // zbytek prace

        if (verbose || verboseProcessCommunication) cout << myPrefix << "Received SOME work from process " << status.MPI_SOURCE << endl;

        return FSin;
    }
}

void sendToken(char tokenColor) { // prijemce je pevne dany
    int nextProcess = (myID + 1) % noIDs;

    if (verbose || verboseProcessCommunication) cout << myPrefix << "Sending token to process " << nextProcess << "." << endl;

    MPI_Send(&tokenColor, 1, MPI_CHAR, nextProcess, MSG_TOKEN, MPI_COMM_WORLD);

    if (verbose || verboseProcessCommunication) cout << myPrefix << "Sent " << TOKEN_COLOR(tokenColor) << " token to process " << nextProcess << "." << endl;
}

/* 
 *  Volat jenom pokud maji token
 *  return: WHITE, BLACK, -1 což znamena ze se ma ukončit vypočet
 */
char getColorOfSendingToken(char colorOfReceivedToken, bool haveWork, bool &workSentToLower) {
    char colorOfSendingToken;

    if (AM_MASTER) {
        if (colorOfReceivedToken == WHITE) { // vratil se mi bilej takze nikdo nema praci a je nutne ukoncit algoritmus
            return -1;
        } else { // prisel cernej takze ho obarvim podle toho jeslti mam praci (mensimu jsem nemohl poslat pokud jsem 0)
            if (haveWork || workSentToLower) {
                colorOfSendingToken = BLACK;
            } else {
                colorOfSendingToken = WHITE; // restartuje posilani tokenu
            }
        }
    } else { // Slaves obarvi podle toho jeslti maji praci a ejslti nahoodu neposlali mensimu, jinak nechaji puvodni barvu
        if (haveWork || workSentToLower) {
            colorOfSendingToken = BLACK;
        } else { // else necham původní
            colorOfSendingToken = colorOfReceivedToken;
        }
    }
    workSentToLower = false; // vynulovat
    return colorOfSendingToken;
}

char receiveToken() { // odesilatel je pevne dany ale pro zjednoduseni prijimam ode vsech
    char tokenColor;
    MPI_Status status;

    if (verbose || verboseProcessCommunication) cout << myPrefix << "Receiving token." << endl;

    MPI_Recv(&tokenColor, 1, MPI_CHAR, MPI_ANY_SOURCE, MSG_TOKEN, MPI_COMM_WORLD, &status);

    if (verbose || verboseProcessCommunication) cout << myPrefix << "Received " << TOKEN_COLOR(tokenColor) << " token from process " << status.MPI_SOURCE << "." << endl;

    return tokenColor;
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

void sendSolution(Field* bestSolution, int noStackDivisions, int noAlgorithmIteration, MPI_Request* lastRequest, bool &lastRequestValidity) { // prijemce je vzdy Master
    int pos = 0;

    if (verbose || verboseProcessCommunication) cout << myPrefix << "Sending best Field to Master." << endl;

    waitForLastWorkRequest(lastRequest, lastRequestValidity);

    char isNull;
    if (bestSolution != NULL) {
        isNull = 0;

        MPI_Pack(&isNull, 1, MPI_CHAR, workBuffer, WORK_BUFFER_SIZE, &pos, MPI_COMM_WORLD);
        bestSolution->pack(workBuffer, WORK_BUFFER_SIZE, &pos);
        MPI_Pack(&noStackDivisions, 1, MPI_INT, workBuffer, WORK_BUFFER_SIZE, &pos, MPI_COMM_WORLD);
        MPI_Pack(&noAlgorithmIteration, 1, MPI_INT, workBuffer, WORK_BUFFER_SIZE, &pos, MPI_COMM_WORLD);

        MPI_Isend(workBuffer, WORK_BUFFER_SIZE, MPI_PACKED, MASTER, MSG_SOLUTION, MPI_COMM_WORLD, lastRequest);
        lastRequestValidity = true;

        if (verbose || verboseProcessCommunication) cout << myPrefix << "Sent SOME best Field to Master." << endl;
    } else { // nema zadne reseni
        isNull = 1;

        MPI_Pack(&isNull, 1, MPI_CHAR, tinyBuffer, TINY_BUFFER_SIZE, &pos, MPI_COMM_WORLD);
        MPI_Pack(&noStackDivisions, 1, MPI_INT, tinyBuffer, TINY_BUFFER_SIZE, &pos, MPI_COMM_WORLD);
        MPI_Pack(&noAlgorithmIteration, 1, MPI_INT, tinyBuffer, TINY_BUFFER_SIZE, &pos, MPI_COMM_WORLD);

        MPI_Send(tinyBuffer, TINY_BUFFER_SIZE, MPI_PACKED, MASTER, MSG_SOLUTION, MPI_COMM_WORLD);

        if (verbose || verboseProcessCommunication) cout << myPrefix << "Sent NULL best Field to Master." << endl;
    }
}

/* in/out lastRequestValidity, lastRequestValidity is changed */
Field* receiveSolution(int &noStackDivisions, int &noAlgorithmIterations, MPI_Request* lastRequest, bool &lastRequestValidity) { // odesilatel muze bejt kdokoliv
    Field* field;
    char isNull;
    int pos = 0;
    int noSD, noAI;
    MPI_Status status;

    if (verbose || verboseProcessCommunication) cout << myPrefix << "Receiving best Field." << endl;

    waitForLastWorkRequest(lastRequest, lastRequestValidity);

    MPI_Recv(workBuffer, WORK_BUFFER_SIZE, MPI_PACKED, MPI_ANY_SOURCE, MSG_SOLUTION, MPI_COMM_WORLD, &status);

    MPI_Unpack(workBuffer, WORK_BUFFER_SIZE, &pos, &isNull, 1, MPI_CHAR, MPI_COMM_WORLD);
    if (isNull == 0) {
        field = Field::unpack(workBuffer, WORK_BUFFER_SIZE, &pos);
    } else {
        field = NULL;
    }

    MPI_Unpack(workBuffer, WORK_BUFFER_SIZE, &pos, &noSD, 1, MPI_INT, MPI_COMM_WORLD);
    noStackDivisions = noStackDivisions + noSD;
    MPI_Unpack(workBuffer, WORK_BUFFER_SIZE, &pos, &noAI, 1, MPI_INT, MPI_COMM_WORLD);
    noAlgorithmIterations = noAlgorithmIterations + noAI;

    if (isNull == 0) {
        if (verbose || verboseProcessCommunication) cout << myPrefix << "Received SOME best Field from process " << status.MPI_SOURCE << "." << endl;
    } else {
        if (verbose || verboseProcessCommunication) cout << myPrefix << "Received NULL best Field from process " << status.MPI_SOURCE << "." << endl;
    }

    return field;
}

// prvni 4 argumenty povinne, ostatni teoreticky nejsou
/*
void handleMessage(int source, int tag, int *flag, MPI_Status *status, Field* &currSolution, FieldStack* &stack, Field* &bestSolution, bool &finishFlag, bool &haveToken, char &tokenColor, MPI_Request* lastRequest, bool &lastRequestValidity) {

    MPI_Iprobe(source, tag, MPI_COMM_WORLD, &flag, &status);

    if (flag) { // prisla zprava

        switch (status.MPI_TAG) {
            case MSG_WORK_REQUEST:
                receiveWorkRequest(status.MPI_SOURCE);

                if (stack != NULL) { // pokusim se poslat praci
                    FieldStack* FSout = stack->divide();
                    sendWorkResponse(FSout, bestSolution, status.MPI_SOURCE, lastRequest, lastRequestValidity);
                    delete FSout;
                } else { // urcite nemam praci
                    sendNoWorkResponse(comm_status.MPI_SOURCE); // nemam dostatek prace
                }
                break;

            case MSG_WORK_RESPONSE: // prisla mi zpatky odpoved (nemusi v ni bejt ale prace)
                delete stack;
                stack = receiveWorkResponse(bestSolution, MPI_ANY_SOURCE, lastRequest, lastRequestValidity); // prijde mi od donora nikdo jinej mi ji poslat nemuze
                break;

            case MSG_TOKEN:
                tokenColor = receiveToken();
                if (stack == NULL || stack->isEmpty()) { // mam token a prazdnej stack -> zase hned odpovim
                    tokenColor = getColorOfSendingToken(tokenColor, false, workSentToLower);
                    if (tokenColor == -1) { // Master dostal bilej token takze nikdo nema praci a je treba ukoncit algoritmus
                        for (int i = 1; i < noIDs; i++) { // send finish message to all other process
                            sendFinish(i);
                        }
                        finishFlag = true; // skonci
                    } else {
                        sendToken(tokenColor);
                        haveToken = false; // udelat venku
                    }
                } else { // nemam prazdnej stack takze si token necham
                    haveToken = true;
                }
                break;

            case MSG_FINISH:
                receiveFinish();
                finishFlag = true; // skonci

            case MSG_SOLUTION:
                // gatheredSolutins++; // udelat venku
                currSolution = receiveSolution(lastRequest, lastRequestValidity);
                improveSolution(bestSolution, currSolution);
                return 0;

            default:
                if (verbose || verboseProcessCommunication) cout << myPrefix << "Not known tag " << status.MPI_TAG << " from process " << status.MPI_SOURCE << "." << endl;
                return 0;
        }
    }
}*/

/**
 * 
 * $ ./generator a b n > gen.txt; ./transform.sh a b n gen.txt > trans.txt; mpirun -np N ./ppr -f trans.txt
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

    bool firstTokenSent = false;
    char tokenColor = false; // true == white (no work), false == black (have work), -1 v pripade ze se ma ihned skoncit, char taky proto ze nejde posilat bool
    bool haveToken = false;
    bool workSentToLower = false; // true if Pj sent data to Pi where i < j, false after token sent

    bool finishFlag = false;

    int noGatheredSolutins = 0;
    int noStartedIDs = 1; // Master vzdy
    int noStackDivisions = 0;
    int noAlgorithmIteration = 0;

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
            initField(myCurrField);
        } catch (string ex) {
            cout << "Exception: " << ex << endl;

            delete myCurrField; // clean-up
            delete myBestField; // clean-up
            delete myStack; // clean-up
            exit(EXIT_FAILURE);
        }

        cout << "-------------------- TASK --------------------" << endl;
        cout << "Number of given processors: " << noIDs << endl;
        cout << myCurrField->toString(true, false);
        cout << "-------------------- /TASK --------------------" << endl;

        sendToken(BLACK);
        haveToken = false;

    } else { // slaves wait (blocking way) for first data from master
        processArguments(argc, argv); // need to get verbose flags

        if (verbose || verboseProcessCommunication) cout << myPrefix << "Waiting for start Field." << endl;

        /*
         * HANDLE MESSAGES - klasicky odpovidat na vsechno a cekat nez dostanu prvni praci
         */
        bool receivedResponse;
        while (true) { // abych pockal na work response

            if (verbose || verboseProcessCommunication) cout << myPrefix << "Handle messages." << endl;

            receivedResponse = false;
            do { // mohlo prijit vic pozadavku

                // dva stejny/podobny kody - bylo by fajn to mit takhle nez na dvou mistech to stejny ale je to hrozna funkce
                // handleMessage(MPI_ANY_SOURCE, MPI_ANY_TAG, int* flag, MPI_Status* status, Field*& currSolution, FieldStack*& stack, Field*& bestSolution, bool& finishFlag, bool& haveToken, char& tokenColor, MPI_Request* lastRequest, bool& lastRequestValidity)

                MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &comm_flag, &comm_status);

                if (comm_flag) { // prisla zprava

                    switch (comm_status.MPI_TAG) {
                        case MSG_WORK_REQUEST:
                            receiveWorkRequest(comm_status.MPI_SOURCE);
                            sendNoWorkResponse(comm_status.MPI_SOURCE); // nemam dostatek prace
                            break;

                        case MSG_WORK_RESPONSE: // dostavam start FS - musi tam nejaka bejt to posila Master
                            receivedResponse = true;
                            delete myStack;
                            myStack = receiveWorkResponse(myBestField, 0, &comm_request, comm_requestValidity); // prace pouze od 0
                            myCurrField = myStack->pop(); // rovnou si nactu prvni field abych ho mohl zacit resit
                            comm_flag = 0; // ostatni doresim az zase nebudu mit praci
                            break;

                        case MSG_TOKEN:
                            tokenColor = receiveToken();
                            haveToken = true; // mam token a hned se ho zase zbavim jestli jeste nemam praci, jinak mi zustane
                            if (myStack == NULL || myStack->isEmpty()) {
                                // myslim si ze bych mel ihned odpovedet pokud mam porad prazdnej buffer aby se to necyklilo
                                tokenColor = getColorOfSendingToken(tokenColor, false, workSentToLower);
                                if (tokenColor == -1) { // Master dostal bilej token takze nikdo nema praci a je treba ukoncit algoritmus
                                    for (int i = 1; i < noIDs; i++) { // send finish message to all other process
                                        sendFinish(i);
                                    }
                                    finishFlag = true; // finish sam pro sebe
                                    comm_flag = 0; // teď už je to v pohodě nikde necekam blokujicne, neni nutne ani doresit zbytek zprav protoze stejne vsichni skonci
                                } else {
                                    sendToken(tokenColor);
                                    haveToken = false;
                                }
                            }
                            break;

                        case MSG_FINISH:
                            receiveFinish();

                            comm_flag = 0; // uz by mozna nemusel doresovat ani ostatni zpravy protoze kdyz MSG_FINISH dostane jeden dostanou ji vsichni
                            finishFlag = true; // ukoncujici podminka celeho algoritmu
                            break;

                        default:
                            if (verbose || verboseProcessCommunication) cout << myPrefix << "Not known or not expected tag " << comm_status.MPI_TAG << " from process " << comm_status.MPI_SOURCE << "." << endl;
                            break;
                    }
                }
            } while (comm_flag);

            if (finishFlag) { // nedostal jsem jeste ani praci a mam skoncit
                break;
            }

            if (!receivedResponse) { // jeste jsem nedostal odpoved na zadost o praci -> je treba pockat na odpoved
                if (verbose || verboseProcessCommunication) cout << myPrefix << "No more messages, but still do not have start FieldStack. Sleep..." << endl;
                usleep(SLEEP_TIME);
                continue;
            } else { // dostale jsem praci jdu zacit resit
                break;
            }
        }
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
                myCurrField->solveRectShapes(myStack, myBestField); // vždy existuje alespoň jeden tvar
            }

            /*
             * (1+tvar) (2)
             */
            if (myCurrField->getRectangles()->getCurrent()->hasPosition() == false) {
                /*
                 * Řeší pozice aktuálního obdélníku.
                 * První pozici použije pro tento field, ostatní pro nové fieldy které vloží na stack.
                 */
                if (myCurrField->solveRectPositions(myStack, myBestField) == false) { // neexistuje žádná možná pozice => končím DFS, řešení nenalezeno
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
        delete myCurrField; // uz ho nepotrebuju
        myCurrField = myStack->pop(); // chci nove

        while (myCurrField == NULL) { // stack is empty - nemam svoje nove, zadam ostatni

            if (noIDs == 1) { // is not parallel
                finishFlag = true; // ukoncujici podminka
                break; // vyskoci z totohle whilu kterej zada o praci // sekvencne nema koho pozadat
            }

            // 1) vyresit token - pripadne ukonceni vypoctu
            // 2) pozadat o praci - aby mohl pokracovat
            // 3) obsluhovat prichazejici zpravy - dokola aby nedoslo k deadlocku

            /* 
             * 1) TOKEN
             */
            if (haveToken) {
                tokenColor = getColorOfSendingToken(tokenColor, false, workSentToLower);
                if (tokenColor == -1) { // Master dostal bilej token takze nikdo nema praci a je treba ukoncit algoritmus
                    for (int i = 1; i < noIDs; i++) { // send finish message to all other process
                        sendFinish(i);
                    }
                    finishFlag = true; // finish sam pro sebe
                    break; // teď už je to v pohodě nikde necekam blokujicne, neni nutne ani doresit zbytek zprav protoze stejne vsichni skonci
                } else {
                    sendToken(tokenColor);
                    haveToken = false;
                }
            }

            /*
             *  2) DONOR REQUEST
             */
            int donor = getDonor();
            sendWorkRequest(donor);

            /*
             *  3) HANDLE MESSAGES
             */
            bool receivedResponse;
            while (true) { // abych pockal na work response

                if (verbose || verboseProcessCommunication) cout << myPrefix << "Handle messages." << endl;

                receivedResponse = false;
                do { // mohlo prijit vic pozadavku

                    // bylo by fajn to mit takhle nez na dvou mistech to stejny ale je to hrozna funkce
                    // handleMessage(MPI_ANY_SOURCE, MPI_ANY_TAG, int* flag, MPI_Status* status, Field*& currSolution, FieldStack*& stack, Field*& bestSolution, bool& finishFlag, bool& haveToken, char& tokenColor, MPI_Request* lastRequest, bool& lastRequestValidity)

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
                                myStack = receiveWorkResponse(myBestField, donor, &comm_request, comm_requestValidity);
                                // comm_flag = 0; // ostatni zpravy doresim zase az priste nebudu mit praci
                                break;

                            case MSG_TOKEN:
                                tokenColor = receiveToken();
                                haveToken = true; // mam token a hned se ho zase zbavim jestli jeste nemam praci, jinak mi zustane
                                if (myStack == NULL || myStack->isEmpty()) {
                                    // myslim si ze bych mel ihned odpovedet pokud mam porad prazdnej buffer aby se to necyklilo
                                    tokenColor = getColorOfSendingToken(tokenColor, false, workSentToLower);
                                    if (tokenColor == -1) { // Master dostal bilej token takze nikdo nema praci a je treba ukoncit algoritmus
                                        for (int i = 1; i < noIDs; i++) { // send finish message to all other process
                                            sendFinish(i);
                                        }
                                        finishFlag = true; // finish sam pro sebe
                                        comm_flag = 0; // teď už je to v pohodě nikde necekam blokujicne, neni nutne ani doresit zbytek zprav protoze stejne vsichni skonci
                                    } else {
                                        sendToken(tokenColor);
                                        haveToken = false;
                                    }
                                }
                                break;

                            case MSG_FINISH:
                                receiveFinish();

                                comm_flag = 0; // uz by mozna nemusel doresovat ani ostatni zpravy protoze kdyz MSG_FINISH dostane jeden dostanou ji vsichni
                                finishFlag = true; // ukoncujici podminka celeho algoritmu
                                break;

                                /* po finish flagu hned konci takze je vsechny sesbira az nakonci
                               case MSG_SOLUTION:
                                   noGatheredSolutins++;
                                   myCurrField = receiveSolution(noStackDivisions, noAlgorithmIteration, &comm_request, comm_requestValidity);
                                   improveSolution(myBestField, myCurrField);
                                   break;
                                 */

                            default:
                                if (verbose || verboseProcessCommunication) cout << myPrefix << "Not known or not expected tag " << comm_status.MPI_TAG << " from process " << comm_status.MPI_SOURCE << "." << endl;
                                break;
                        }
                    }
                } while (comm_flag);

                if (finishFlag) { // vsichni konci, nemusim uz nic resit a skoncim
                    break;
                }

                if (!receivedResponse) { // jeste jsem nedostal odpoved na zadost o praci -> je treba pockat na odpoved
                    if (verbose || verboseProcessCommunication) cout << myPrefix << "No more messages, but still do not have work response. Sleep..." << endl;
                    usleep(SLEEP_TIME);
                    continue;
                } else { // dostale jsem praci jdu pokracovat v reseni
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

        } // konec zadani o prace, dostal jsem praci, nebo jsem ji mel uz od zacatku

        if (finishFlag) { // neni treba uz pokracovat
            break;
        }

        /*
         * Prvotní rozposláni start FieldStacků
         */
        if (AM_MASTER && noStartedIDs < noIDs && myStack != NULL) {

            
            //FieldStack* FSout = myStack->divideByOne();
            FieldStack* FSout = myStack->divide();

            while (FSout != NULL) {
                if (verbose || verboseProcessCommunication) cout << myPrefix << "Sending start FieldStack to process " << noStartedIDs << "." << endl;

                sendWorkResponse(FSout, myBestField, noStartedIDs, &comm_request, comm_requestValidity);
                noStartedIDs++;

                delete FSout;
                if (noStartedIDs < noIDs) { // jestli je jeste komu poslat prvotni praci tak zkus znova rozdelit
                    FSout = myStack->divideByOne();
                } else { // jestli uz neni komu posilat tak uz praci nedel! tim skonci i while
                    FSout = NULL; // vyskoci z podminky;
                }
            }
        }

        /*
         * Test jen na zadost o praci, ostatni resim pri prazdnym stacku
         */
        noAlgorithmIteration++;
        if (noAlgorithmIteration % WORK_REQUEST_CHECK_FREQUENCY == 0) {

            if (verbose || verboseProcessCommunication) cout << myPrefix << "Handle work requests." << endl;

            bool anyRequest = false;
            do { // mohlo prijit vic pozadavku
                MPI_Iprobe(MPI_ANY_SOURCE, MSG_WORK_REQUEST, MPI_COMM_WORLD, &comm_flag, &comm_status);
                if (comm_flag) { // prisel pozadavek na praci
                    anyRequest = true;

                    receiveWorkRequest(comm_status.MPI_SOURCE);

                    FieldStack * FSout = myStack->divide();
                    noStackDivisions++;
                    sendWorkResponse(FSout, myBestField, comm_status.MPI_SOURCE, &comm_request, comm_requestValidity);

                    if (comm_status.MPI_SOURCE < myID) {
                        workSentToLower = true;
                    }

                    delete FSout;
                }
            } while (comm_flag);

            if (anyRequest == false) {
                if (verbose || verboseProcessCommunication) cout << myPrefix << "NO work requests." << endl;
            }
        }
    }

    /* waiting for all process, then end */ // na všechny procesy počkám tím že od nich přijmu jejich nej řešení
    // MPI_Barrier(MPI_COMM_WORLD); 

    /* Master */
    if (AM_MASTER) { // Master počkám na zprávy od Slaves a vypíšu řešení
        if (verbose || verboseProcessCommunication) cout << myPrefix << "Gathering rest of best Fields from other processes." << endl;

        for (int i = 1; i < noIDs - noGatheredSolutins; i++) { // pockam jeste na zbytek reseni (ne ty ktery neprisli kdyz jsem obsluhoval posledni zpravy)
            myCurrField = receiveSolution(noStackDivisions, noAlgorithmIteration, &comm_request, comm_requestValidity);
            improveSolution(myBestField, myCurrField);
        }

        cout << "-------------------- SOLUTION --------------------" << endl;
        if (myBestField != NULL) {
            cout << myBestField->toString(true, false);
        } else {
            cout << "Solution does not exist!" << endl; // muze nastat
        }
        /* time measuring - stop */
        t_end = MPI_Wtime();

        cout << "Calculation took: " << (t_end - t_start) << " sec." << endl <<
                "Number of given processors: " << noIDs << endl <<
                "Number of used processors: " << noStartedIDs << endl <<
                "Number of outer iterations (no DFSs): " << noAlgorithmIteration << endl <<
                "Number of stackDivisions: " << noStackDivisions << endl;
        cout << "-------------------- /SOLUTION --------------------" << endl;

    } else { // Slaves pošlou svoje řešení
        sendSolution(myBestField, noStackDivisions, noAlgorithmIteration, &comm_request, comm_requestValidity);
    }

    delete myCurrField; // clean-up
    delete myBestField; // clean-up
    delete myStack; // clean-up

    /* shut down MPI */
    MPI_Finalize();

    exit(EXIT_SUCCESS);
}
