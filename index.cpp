#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <string.h>

#include "Field.h"
#include "FieldStack.h"

using namespace std;

const char *fileName = "input.txt";
bool verbose = false;

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

    in.close();
}

int processArguments(int argc, char** argv) {
    for (int i = 1; i < argc; i++) { // argv[0] executable name
        if (strcmp(argv[i], "-v") == 0) {
            verbose = true;
        } else if (strcmp(argv[i], "-f") == 0) {
            fileName = argv[i + 1];
            i++;
        } else if (strcmp(argv[i], "-h") == 0) {
            cout << "Usage:" << endl <<
                    "\t-v\t\tfor verbose" << endl <<
                    "\t-f \"file\"\tto specific input file, default is \"input.txt\"" << endl <<
                    "\t-h\t\tfor this help" << endl;
            exit(EXIT_SUCCESS); // @TODO do clean-up
        } else {
            cout << argv[i] << " - no such parameter" << endl;
            exit(EXIT_FAILURE);
        }
    }
}

/**
 * Usage:
 *      -v              for verbose 
 *      -f "file"       to specific input file, default is "input.txt"
 *      -h              for this help
 */
int main(int argc, char** argv) {
    FieldStack stack; // use an implicit constructor to initialise stack pointers & size
    Field* field = NULL;
    Field* bestField = NULL;

    processArguments(argc, argv);
    try {
        initField(field, fileName);
    } catch (string ex) {
        cout << "Exception: " << ex << endl;

        delete field;
        exit(EXIT_FAILURE); // @TODO do clean-up
    }

    cout << "---------- TASK ----------" << endl;
    cout << field->toString();

    /* test */

    /* ALGORITMUS Z EDUXU 
     * 
    pro všechna nenulová čísla v mřížce, opakuj
        vyber (zatím nepoužité) nenulové číslo x z mřížky
        pro všechny čísla rx a ry (rozměry obdélníku) takové, že rx*ry=x dělej:
        pro všechny px a py (pozice leveho horniho rohu obdélniku) takové, že px+rx < = a a py+ry < = b dělej
            pokud překryv s předchozímí obdélníky, zkus jiné px a py
            obdélník musí obsahovat právě jedno nenulové číslo x, jinak zkus jiné px a py
            pokud žádné px a py nevyhovuje ⇒ návrat.
            pokud px a py nalezeno a jednalo se o poslední obdélník ⇒ nalezeno řešení
    Pokud prohledán stavový prostor, ⇒ nemá řešení.
     */

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
                    delete bestField;
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
        field = stack.pop();
        if (field == NULL) {
            break; // sequential
            // parallel has to ask other processors
        }
    }

    cout << "---------- SOLUTION ----------" << endl;
    if (bestField != NULL) {
        cout << bestField->toString();
    } else {
        cout << "Solution does not exist!" << endl; // předpokládám že by nemělo nastat pokud projde podmínkou v initField
    }

    delete bestField;
    exit(EXIT_SUCCESS); // @TODO do clean-up

    /* POZNÁMKY
     * 
     * řešit ořezávání pomocí nejlepšího řešení/absolutně nej řešení, aktuální testovat jestli už náhodou není horší než zatím nejlepší - asi lze úplně vynechat nepřinese to podle mě moc velké zrychlení
     * 
     * není tam nikce moc ošetření na NULL pointery
     * 
     * Mít správně destruktory,
     * 
     * Pro nějaký to lepší půlení zásobníku bysme mohli ukládat číslo kolik pod sebou ještě má nevyřešenejch Rect a tím by se to dalo docela hezky půlit. Vědět kolik je celkovej součet a potom jít odzadu dokud ho nepřesáhnu.
     * 
     * Udělat to skutečně jako stack. dole budou největší úlohy a ty budu uřezávat a posílat jinému procesoru
     * je to lepší asi protože budu rychlejc získávat řešení a můžu tak líp ořezávat ty ostatní případně skončit pokud najdu nejlepší
     * 
     * Nešlo by nějak pamatovat si toho míň než celej Field? Asi zbytečný to řešit pokud nebude problém s pamětí
     * 
     */
}
