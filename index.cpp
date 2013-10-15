#include <iostream>
#include <fstream>
#include <cstdlib>

#include "Field.h"
#include "FieldStack.h"

using namespace std;

const char *fileName = "vstup.txt";

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
        throw "Could not open a file!";
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
    if (n != field->getRectangles()->getSize()) {
        throw "Given n (number of non-zero numbers) does not correspond to actual non-zero numbers in given field!";
    }
    if (field->getDimension().getX() * field->getDimension().getY() != field->getRectangles()->getAreaSum()) {
        throw "Given rectangles areas (non-zero numbers) do not cover whole field area!";
    }

    in.close();
}

int main(void) {
    FieldStack stack; // use an implicit constructor to initialise stack pointers & size
    Field* field;
    Field* bestField = NULL;

    try {
        initField(field, fileName);
        cout << "----- TASK -----" << endl;
        cout << field->toString();
    } catch (const char* ex) {
        cout << "Exception: " << ex << endl;
        return (EXIT_FAILURE);
    }

    
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
                cout << "currentRect = NULL\n";
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
                cout << "solving RectShapes for rectangle at: " << field->getRectangles()->getCurrent()->getBasePosition().toPointString() << endl;
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
                cout << "solving RectPositions for rectangle at: " << field->getRectangles()->getCurrent()->getBasePosition().toPointString() << endl;
                /*
                 * Řeší pozice aktuálního obdélníku.
                 * První pozici použije pro tento field, ostatní pro nové fieldy které vloží na stack.
                 */
                if (field->solveRectPositions(stack) == false) { // neexistuje žádná možná pozice => končím DFS, řešení nenalezeno
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

    cout << "----- SOLUTION -----" << endl;
    if (bestField != NULL) {
        cout << bestField->toString();
    } else {
        cout << "Solution does not exist!" << endl; // předpokládám že by nemělo nastat pokud projde podmínkou v initField
    }

    delete bestField;

    /* POZNÁMKY
     * 
     * řešit ořezávání pomocí nejlepšího řešení, aktuální testovat jestli už náhodou není horší než zatím nejlepší - asi lze úplně vynechat nepřinese to podle mě moc velké zrychlení
     * 
     * není tam nikce moc ošetření na NULL pointery
     * 
     * Mít správně destruktory, kopírující konstruktory
     * 
     * Pro nějaký to lepší půlení zásobníku bysme mohli ukládat číslo kolik pod sebou ještě má nevyřešenejch Rect a tím by se to dalo docela hezky půlit. Vědět kolik je celkovej součet a potom jít odzadu dokud ho nepřesáhnu.
     * 
     * Udělat to skutečně jako stack. dole budou největší úlohy a ty budu uřezávat a posílat jinému procesoru
     * je to lepší asi protože budu rychlejc získávat řešení a můžu tak líp ořezávat ty ostatní případně skončit pokud najdu nejlepší
     * 
     * Nešlo by nějak pamatovat si toho míň než celej Field? Asi zbytečný to řešit pokud nebude problém s pamětí
     * 
     *  tabulku vyplňovat číslem vyplňované dlaždice ať vidíme jak to nakonec vypadá to rozdělení
     */
}
