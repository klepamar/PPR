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
bool initField(Field* &field, const char* &fileName) {
    int a, b; // a, b - dimension of field
    int n; // number of non-zero numbers
    string dummy;
    ifstream in;

    // open file
    in.open(fileName);
    if (!in.is_open()) {
        cerr << "Could not open \"" << fileName << "\"!" << endl;
        return (EXIT_FAILURE);
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
        throw "Given n (number of non-zero numbers) does not correspond to actual non-zero numbers!";
    }

    in.close();
}

int main(void) {
    FieldStack stack;
    Field* field;

    try {
        initField(field, fileName);
    } catch (const char* ex) {
        cout << ex << endl;
    }
    field->showField();

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

    /* NÁŠ ALGORITMUS
     * 
     *  myField.addShapedRectToStack() // předpřipraví na stack obdélník který je na řadě (jen 2. a další tvary)
     *          shapes = rectangles[i].getShapes();
     *          pro každý tvar vyrob kopii Field a přidej právě zpracovávanému rectangle tvar
     *          Všechny tyto Field ulož na stack (na stacku je několik větví, každá s jiným tvarem ale pozice zatím nedefinovány)
     * 
     *  while(stack is not empty)
     *          pokud už budu mít co řešit (1. tvar aktuálního obdélníku) tak while nekonečnej a načtení stacku na konci s podmínkou ukončení
     * 
     *          //Stack.top().solve() // vyber z stacku první Field a pokračuj v řešení - Protraverzuje strom až do nejlevějšího možného konce, možnosti jít vpravo ukládá na stack (nakopíruje tam celej field ve kterém jsou info kde pokračovat)
     *          //        vezme aktuální obdélník (už má konkrétní tvar) a začne (pokračuje) ve zkoušení pozic
     *          //        bere pozice a kontroluje jestli jsou v pořádku (nesmí překrývat předchozí obdélník (nenulové políčko) ani žádný následující obdélník (nenulové políčko))
     * 
     *          while(nenajde řešení)
     *                  pokud najde vyhovující řešení 
     *                          označí obdálník za vyřešenej, zapíše ho (vybarví Field číslem obdélníku), 
     *                          ?? spočítá součet obvodů (přičte aktuální obdélník)
     *                          (uvnitř rectangle) posune se v řešení (označí další pozici která se bude vyhledávat - asi "skoro automaticky" nějakým zvyšováním proměnných ve for cyklech) a takovou Field uloží na stack (bude jasné kde pokračovat až to někdo z stacky vezme)
     *                          pokud další pozice neexistuje na stacku nic neukládá
     *                          provede myField.addShapedRectToStack() (přídá novej obdélník do tohohle řešení) a končí; // tohle možná úplně nedodržuje to že půjdu do hloubky ale snad by to teoreticky nemuselo vadit, Ale asi je to jedno protože v tomhle případu stejně musím projít naprosto celej stavovej prostor
     *                          lepší než končit nepřidávat na stack všechny tvary ale jen 2. a další.
     *                          první budu rovnou dál řešit takže provedu celej DFS bez uložení na stack
     *                          řešení ve kterým chci pokračovat neukládám na stack to je zbytečný ale normálně v něm pokračuju
     *                  
     *                  pokud nenajde vyhovující pozici
     *                          končí DFS
     *                          tak tento průchod nemá řešení.
     *                          končí a pokračuje načtením ze stacku
     *          
     *          Field = Stack topPop()
     *          if(field == null) {
     *                  nemám práci (sek končím, paral žádám)
     *          }
     */

    while (false /* TODO true */) { // nový DFS, stav sebrán ze stacku
        while (true) { // provedení DFS až do konce
            /**
             *  řeší tvar aktuálního obdélníku, první tvar použije pro tento field ostatní pro nové fieldy které vloží na stack
             *  pokud je treba doresit pozice tak neresit tvar ale rovnou return
             */
            field->solveRectShape();

            /**
             * mám tvar - nový nebo nedořešený
             * řeším pozice zkoušením všech možností a kontrolováním přípustnosti (překrývá jen nulová pole)
             *
             * pokud najdu řešení tak označit, posunout se, přičíst obvod, zkontrolovat smysl řešení - uvnitř funkce
             * pokud nenajdu řešení konec DFS, pokud nema cenu pokračovat tak konec DFS
             */
            if (field->solveRectPos() == false) { // nenašel řešení => konec DFS
                break;
            }
        }

        /**
         * načtení dalšího stavu k řešení
         * ukončující podmínka
         */
        field = stack.top();
        if (field == NULL) {
            break; // sequential
            // parallel has to ask other processors
        }
    }


    /* POZNÁMKY
     * 
     * Mít správně destruktory
     * 
     * Pro nějaký to lepší půlení zásobníku bysme mohli ukládat číslo kolik pod sebou ještě má nevyřešenejch Rect a tím by se to dalo docela hezky půlit. Vědět kolik je celkovej součet a potom jít odzadu dokud ho nepřesáhnu.
     * 
     * Udělat to skutečně jako stack. dole budou největší úlohy a ty budu uřezávat a posílat jinému procesoru
     * je to lepší asi protože budu rychlejc získávat řešení a můžu tak líp ořezávat ty ostatní případně skončit pokud najdu nejlepší
     * 
     * Pozor s tím Stackem a ukládáním na stack jenom 2. a dalšího tvaru (dopře podmínky zkončení, začítku cyklů)
     * možná načtení stacku na konec whilu
     * 
     * Nešlo by nějak pamatovat si toho míň než celej Field? Asi zbytečný to řešit pokud nebude problém s pamětí
     * 
     * Teoreticky pokud znám už nějaké řešení můžu pro svoje řešení počítat obvody a pokud je větší tak to rovnou utnout a nepočítat dál
     * 
     * Field vlastně plně udává stav toho algoritmu (vyřešený obdélníky jsou zanesený, na první nedořešenej ukazuje pointer a uchovává si vyzkoušený pozice)
     * Do Stacku se ukládá Field ve kterém je jaký tvar se zpracovává a jaká pozice je na řadě
     * 
     * Ten Stack nemůže bít uvnitř Fieldu. Pořád by se kopírovala. Musí být svým způsobem globální. Nebo aspoň definována na začátku mainu
     * Je třeba vyřešit co se bude používat na tu stacku. Je nutné mít možnost brát první prvek ale i jí například rozdělit a poslat jinému procesoru
     * 
     * Rectangles už mít předpřipravený? a bude v nich vygenerovaný všechny možnosti velikostí i smysluplnejch pozic (aby byl v mezích pole)
     * 
     * melo by bejt jedno v jakým pořadí jsou ty věci na stackě. Jde se co nejvíc dolů a pravý (levý) stromy se dázej na stacku
     * 
     *  clonovat celej Field se všema obdélníkama (deep copy)
     *          tabulka si uchováva kterej obdelnik se ma zpracovavat
     *          obdelnik si uchovava kterej tvar a umistění se má zpracovávat.
     *          (ukládat buď kterej byl provedenej naposledy nebo kterej je teď na řadě)
     *          při vytvoření obdélníku vygenerovat všechny možnosti velikostí i smysluplnejch pozic (aby byl v mezích pole)
     *          tím by snad šlo kdykoliv kdy najdu řešení (a existujou ještě jiná) uložit Field na stacku a z ní to pak kdykoliv vzít a pokračovat (všechno je uloženo ve Field a Rectangles)
     * 
     *  tabulku vyplňovat číslem vyplňované dlaždice ať vidíme jak to nakonec vypadá to rozdělení
     */



}
