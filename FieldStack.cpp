#include <sstream>

#include "FieldStack.h"
#include "Field.h"

using namespace std;

extern bool verbose;

FieldStack::FieldStack() {
    this->size = 0;
    this->topItem = this->bottomItem = NULL;
}

FieldStack::FieldStack(const FieldStack& orig) { // Jakub: myslím že to ani nebudeme potřebovat, budem potřebovat spíš něco jako rozdělení stacku
    this->size = 0; // start from an empty stack
    this->topItem = this->bottomItem = NULL;

    FieldStackItem *origCurrent = orig.bottomItem; // traverse the original stack from the bottom (since we have two pointers available)
    while (origCurrent) {
        push(new Field(*(origCurrent->field))); // ... and push field values for respective FieldStackItem-s
        origCurrent = origCurrent->upper; // move upwards towards the top of the stack
    }
}

FieldStack::~FieldStack() {
    FieldStackItem *current = this->topItem;
    FieldStackItem *nextCurrent = current;
    while (current) // perform the cleanup while stack is not empty from the top to the bottom
    {
        nextCurrent = current->below;
        delete current;
        current = nextCurrent;
    }
    this->topItem = this->bottomItem = NULL;
}

void FieldStack::push(Field* field) {
    // create a new FieldStackItem
    FieldStackItem *newItem = new FieldStackItem(field);

    // place the newly created FieldStackItem onto the top(=bottom) of the stack
    if (isEmpty()) {
        this->topItem = this->bottomItem = newItem;
    } else {
        this->topItem->upper = newItem; // sucasny topItem bude mat nad sebou newItem
        newItem->below = topItem; // newItem bude mat pod sebou sucasny topItem
        topItem = newItem; // newItem sa stane vrcholom stacku
    }

    //increase the stack counter in both cases
    this->size = this->size + 1;
}

Field* FieldStack::pop() {
    if (verbose) cout << "Popping stack:" << endl;

    if (isEmpty()) return NULL; // make sure you check what you have received from the stack!

    FieldStackItem *retValue = topItem; // FieldStackItem to be returned

    if (topItem->below) // popping element from stack with size > 1
    {
        topItem->below->upper = NULL; // currently removed item has some items "below" itself
        topItem = topItem->below; // change topItem
    } else // popping element from stack with size = 1
    {
        this->topItem = this->bottomItem = NULL;
    }
    this->size = this->size - 1;

    if (verbose) cout << retValue->field->toString();

    return retValue->field; // return only the field portion of FieldStackItem
}

bool FieldStack::isEmpty() const {
    if (this->size == 0) return true;
    return false;
}

std::string FieldStack::toString() const {
    ostringstream ss;
    FieldStackItem *tmp = this->topItem;
    
    ss << "<FIELDSTACK>" << endl;
    for (int i = 0; tmp != NULL; i++) {
        ss << "<" << size - 1 << ">" << endl;
        ss << tmp->field->toString();
        ss << "</" << size - 1 << ">" << endl;
        tmp = tmp->below;
    }
    ss << "</FIELDSTACK>" << endl;
    
    return ss.str();
}
