#ifndef FIELDSTACK_H
#define	FIELDSTACK_H

class Field; // instead of include to avoid cycle dependency, no need of Rectangle functions

class FieldStack {
private:
    
    class FieldStackItem {
    public:
        FieldStackItem* next;
        FieldStackItem* prev;
        Field* field;
        FieldStackItem(Field* field);
    };
    
    int size;
    Field* topItem;
    Field* bottomItem; // mohlo by se hodit potom pro to půlení kdy od zadu vezmu nějakou část (asi ne přímo polovinu protože spodní elementy jsou víc u vrchu stromu takže náročnější na výpočet)

public:
    FieldStack();
    FieldStack(const FieldStack& orig);
    virtual ~FieldStack();

    void push(Field* field);
    Field* top(); // pop included
};

#endif	/* FIELDSTACK_H */

