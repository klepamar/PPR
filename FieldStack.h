#ifndef FIELDSTACK_H
#define	FIELDSTACK_H

#include <cstdlib>
class Field; // instead of include to avoid cycle dependency, no need of Rectangle functions

using namespace std;

/**
 * ==3==
 * ==2==
 * ==1==
 * =dno=
 * 
 * stack rastie smerom zvrchu, t.j. FieldStack.pop() vrati hodnotu "3"
 * z pohladu "3" je 3->below = "2"
 * z pohladu "3" je 3->upper = "NULL"
 * 
 */
class FieldStack {
private:

    class FieldStackItem {
    public:
        FieldStackItem* upper;
        FieldStackItem* below;
        Field* field;

        FieldStackItem(Field* field) {
            this->upper = this->below = NULL;
            this->field = field; // call overloaded Field operator= function
        }
    };

    int size;
    FieldStackItem* topItem;
    FieldStackItem* bottomItem; // mohlo by se hodit potom pro to půlení kdy od zadu vezmu nějakou část (asi ne přímo polovinu protože spodní elementy jsou víc u vrchu stromu takže náročnější na výpočet)

public:
    FieldStack();
    FieldStack(const FieldStack& orig);
    virtual ~FieldStack();

    void push(Field* field);
    Field* pop();
    bool isEmpty() const;
};

#endif	/* FIELDSTACK_H */

