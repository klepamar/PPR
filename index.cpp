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
bool initField(Field* &field, const char* fileName) {
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
    in >> a >> b;
    getline(in, dummy); // get rid of new line character
    in >> n;
    getline(in, dummy); // get rid of new line character

    // create new field and fill with data
    field = new Field(Vector2D(a, b));
    field->fill(in);

    // check
    if (n != field->getRectangles()->getSize()) {
        throw "Given n (number of non-zero numbers) does not correspond to actual non-zero numbers in given field!";
    }

    if (field->getDimension().getArea() != field->getRectangles()->getAreaSum()) {
        throw "Given rectangles areas (non-zero numbers) do not cover whole field area!";
    }

    in.close();
}

int main(void) {
    FieldStack stack;
    Field* field;
    Field* bestField = NULL;

    cout << "----- TASK -----" << endl;
    try {
        initField(field, fileName);
        field->showField();
    } catch (const char* ex) {
        cout << "Exception: " << ex << endl;
        return (EXIT_FAILURE);
    }

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

    // předpřipravit abych do DFS šel s tvarem prvního obdélníku
    field->solveRectShapes(stack);

    while (false) { // nový DFS, field ze stacku nebo z init (dva možné stavy - třeba řešit jen pozice třeba řešit tvar a pozice)
        while (true) { // provedení DFS až do konce

            /*
             *  Ukončijící podmínka DFS, řešení nalezeno.
             */
            if (field->getRectangles()->getCurrent() == NULL) { // žadný další rect => končím DFS, řešení nalezeno
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
             * Smyslem kroku je obarvit field jedním konkrétním obdélníkem.
             * Ze stacku můžu dostat dva různé stavy (reprezentovány třídou Field) - (2) aktuální obdélník fieldu má jen tvar nebo (3) aktuální obdélník fieldu má tvar i pozici.
             * Z vlastního předchozího kroku DFS dostanu stav (1) kdy aktuální obdélník fieldu nemá definován ani tvar ani pozici.
             * Takže nad stavem provádím postupně požadované operace (najdu tvary, najdu pozice) dokud není obdélník konkrétní a můžu jím obarvit field.
             * if( (1) ) { řeším tvary }
             * if( (1+tvar) (2) ) { řeším pozice }
             * if( (1+tvar+pozice) (2+pozice) (3) - vždy) { obarvuji }
             */

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
        bestField->showField();
        delete bestField;
    } else {
        cout << "Solution does not exist!" << endl; // předpokládám že by nemělo nastat
    }


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

    /* @TODO
     * Je potřeba tu nějak vhodně udělat ty podmínky ukončení + řešit to že se to pak volá nad fieldama ze stacku všetně těch kde vlastně už není nic k řešení (ošetřeno podmínkou current == NULL)
     * domyslet to do konce s tím když solveRectPos řeší poslední rectangle
     *          ukládám na stack i když vlastně už není co řešit,
     *          jenom s tím pak potřebuju províst tu posledn9 funkcionalitu - zkontrolovat na nej řešení
     *          nějak dostat to označování/vybarvování/posunování ven z funkce solveRectPos sem ale zařídit aby se prováděla jakoby na začátku pro ty fieldy ze stacku
     * 
     * asi to pujde tak jak to teď je jen je to malinko kostrbatý tím že se posunuju k dalšímu rect uvnitř funkce a pro všechny rect naráz i pro ty který já vlastně v tuhle chvíli neřeším
     * 
     * vlastně bych chtěl nějak šíkovně dostat sem funkci Field::markRect(Rectangle*) která vlastně vybarví field, přičte obvod, posune na další rect
     */

}
