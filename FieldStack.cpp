#include <sstream>
#include "mpi.h"

#include "FieldStack.h"
#include "Field.h"

using namespace std;

extern bool verbose;
extern bool verboseStackSize;

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
    if (verbose || verboseStackSize) cout << "Pushing stack: " << this->size << "++" << endl;

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
    if (verbose || verboseStackSize) cout << "Popping stack: " << this->size << "--" << endl;

    if (isEmpty()) {
        if (verbose) cout << "<FIELDSTACK>" << endl << "null" << endl << "</FIELDSTACK>" << endl;
        return NULL; // make sure you check what you have received from the stack!
    }

    FieldStackItem *origTopItem = topItem; // FieldStackItem to be returned
    Field *origTopField = origTopItem->field;

    if (topItem->below) // popping element from stack with size > 1
    {
        topItem->below->upper = NULL; // currently removed item has some items "below" itself
        topItem = topItem->below; // change topItem
    } else // popping element from stack with size = 1
    {
        this->topItem = this->bottomItem = NULL;
    }
    this->size = this->size - 1;

    origTopItem->field = NULL;
    delete origTopItem;

    return origTopField;
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

void FieldStack::pack(void *buffer, int bufferSize, int *bufferPos) { // pozor na ty hvezdicky abych to spravne pouzival pro volani MPI
    if (this->isEmpty()) {
        return;
    }

    MPI_Pack(&(this->size), 1, MPI_INT, buffer, bufferSize, bufferPos, MPI_COMM_WORLD); // size

    // jdu od spodu abych pri unpacku mohl pouzivat push
    FieldStackItem* tmp = bottomItem;
    while (tmp != NULL) {
        tmp->field->pack(buffer, bufferSize, bufferPos);
        tmp = topItem->upper;
    }
}

FieldStack* FieldStack::unpack(void *buffer, int bufferSize, int *bufferPos) {
    int size;
    FieldStack* fieldStack;
    Field* field;

    MPI_Unpack(buffer, bufferSize, bufferPos, &size, 1, MPI_INT, MPI_COMM_WORLD); // size

    fieldStack = new FieldStack(); // sestaveni
    for (int i = 0; i < size; i++) {
        field = Field::unpack(buffer, bufferSize, bufferPos);
        fieldStack->push(field); // topItem, bottomItem, fieldItems
    }

    return fieldStack;
}