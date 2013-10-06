#include <iostream>
#include <fstream>
#include <cstdlib>

#include "Field.h"

using namespace std;

const char *fileName = "vstup.txt";

void readParameters(ifstream &in, int *xDimension, int *yDimension, int *noOfElements) {
    string s;
    in >> *xDimension >> *yDimension;
    getline(in, s); // get rid of new line character
    in >> *noOfElements;
    getline(in, s); // get rid of new line character
    cout << "Dimensions of the field: " << "x=" << *xDimension << ", y=" << *yDimension << endl;
    cout << "Number of non-zero elements: " << *noOfElements << endl;
}

int main(void) {
    int a, b, n;
    ifstream in;
    in.open(fileName);
    if (!in.is_open()) // opening the file with input parameters
    {
        cerr << "Could not open " << fileName << endl;
        return (EXIT_FAILURE);
    }
    readParameters(in, &a, &b, &n);
    Field myField(a, b, n);
    myField.fill(in);
    myField.showField();
    in.close();



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
     *  myField.addShapedRectToStack() // předpřipraví na stack obdélník který je na řadě 
     *          shapes = rectangles[i].getShapes();
     *          pro každý tvar vyrob kopii Field a přidej právě zpracovávanému rectangle tvar
     *          Všechny tyto Field ulož na stack (na stacku je několik větví, každá s jiným tvarem ale pozice zatím nedefinovány)
     * 
     *  while(stack is not empty)
     *          Stack.top().solve() // vyber z stacku první Field a pokračuj v řešení - Protraverzuje strom až do nejlevějšího možného konce, možnosti jít vpravo ukládá na stack (nakopíruje tam celej field ve kterém jsou info kde pokračovat)
     *                  vezme aktuální obdélník (už má konkrétní tvar) a začne (pokračuje) ve zkoušení pozic
     *                  bere pozice a kontroluje jestli jsou v pořádku (nesmí překrývat předchozí obdélník (nenulové políčko) ani žádný následující obdélník (nenulové políčko))
     * 
     *                  pokud najde vyhovující řešení 
     *                          zapíše ho (vybarví Field číslem obdélníku), 
     *                          spočítá součet obvodů (přičte aktuální obdélník)
     *                          (uvnitř rectangle) posune se v řešení (označí další pozici která se bude vyhledávat - asi "skoro automaticky" nějakým zvyšováním proměnných ve for cyklech) a takovou Field uloží na stack (bude jasné kde pokračovat až to někdo z stacky vezme)
     *                          pokud další pozice neexistuje na stacku nic neukládá
     *                          označí obdálník za vyřešenej, provede myField.rectangleToStack() (přídá novej obdélník do tohohle řešení) a končí; // tohle možná úplně nedodržuje to že půjdu do hloubky ale snad by to teoreticky nemuselo vadit, Ale asi je to jedno protože v tomhle případu stejně musím projít naprosto celej stavovej prostor
     *                  
     *                  pokud nenajde vyhovující 
     *                          tak tento průchod nemá řešení.
     *                          končí
     */


    /* POZNÁMKY
     * 
     * Vlastně to není HEAP ale STACK
     * 
     * Nešlo by nějak pamatovat si toho míň než celej Field? Asi zbytečný to řešit pokud nebude problém s pamětí
     * 
     * Teoreticky pokud znám už nějaké řešení můžu pro svoje řešení počítat obvody a pokud je větší tak to rovnou utnout a nepočítat dál
     * 
     * Field vlastně plně udává stav toho algoritmu (vyřešený obdélníky jsou zanesený, na první nedořešenej ukazuje pointer a uchovává si vyzkoušený pozice)
     * Do haldy se ukládá Field ve kterém je jaký tvar se zpracovává a jaká pozice je na řadě
     * 
     * Ta stacka nemůže bít uvnitř Fieldu. Pořád by se kopírovala. Musí být svím způsobem globální. Nebo aspoň definována na začátku mainu
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
