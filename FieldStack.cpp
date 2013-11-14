#include <sstream>
#include "mpi.h"

#include "FieldStack.h"
#include "Field.h"

using namespace std;

extern bool verbose;
extern bool verboseStackSize;
extern string myPrefix;

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

    FieldStackItem *origTopItem = topItem;
    Field *origTopField = origTopItem->field; // field to be returned

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

Field* FieldStack::popBottom() {
    if (verbose || verboseStackSize) cout << "Popping bottom of stack: " << this->size << "--" << endl;

    if (isEmpty()) {
        if (verbose) cout << "<FIELDSTACK>" << endl << "null" << endl << "</FIELDSTACK>" << endl;
        return NULL; // make sure you check what you have received from the stack!
    }

    FieldStackItem *origBottomItem = bottomItem;
    Field *origBottomField = origBottomItem->field; // field to be returned

    if (bottomItem->upper == NULL) { // size == 1, popping last item
        topItem = bottomItem = NULL; // stack will be empty
    } else { // size > 1
        bottomItem->upper->below = NULL;
        bottomItem = bottomItem->upper;
    }
    this->size--;

    origBottomItem->field = NULL;
    delete origBottomItem;

    return origBottomField;
}

bool FieldStack::isEmpty() const {
    if (this->size == 0) return true;
    return false;
}

int FieldStack::getSize() const {
    return size;
}

std::string FieldStack::toString() const {
    ostringstream ss;
    FieldStackItem *tmp = this->topItem;

    if (isEmpty()) {
        ss << "<FIELDSTACK>" << endl << "enpty" << endl << "</FIELDSTACK>" << endl;
        return ss.str();
    }

    ss << "<FIELDSTACK>" << endl;
    for (int i = 0; tmp != NULL; i++) {
        ss << "<" << (size - i - 1) << ">" << endl;
        ss << tmp->field->toString();
        ss << "</" << (size - i - 1) << ">" << endl;
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
        tmp = tmp->upper;
    }
}

FieldStack* FieldStack::divideStack() {
    // return new stack as a result of the procedure and also change current stack
    FieldStack *newStack = new FieldStack();

    // the procedure must be called when multiple elements are present in the stack (!); it does not make sense to divide a stack with 1 item
    if (this->size <= 1) {
        // constructor of FieldStack sets both pointers to NULL, so you can return the new stack directly
        return newStack;
    }

    // find new bottom item of the original stack & top item of the new stack
    FieldStackItem *stackBottom = findNewBottomItem();
    FieldStackItem *stackTop = stackBottom->below;

    // remove the connection between stackBottom and stackTop items
    stackBottom->below = NULL;
    stackTop->upper = NULL;

    // set pointers of the new stack
    newStack->topItem = stackTop;
    newStack->bottomItem = this->bottomItem;

    // change bottom pointer for the original stack
    this->bottomItem = stackBottom;

    // change number of elements within both stacks
    this->recalculateSize();
    newStack->recalculateSize();

    return newStack;
}

void FieldStack::recalculateSize() {
    int sizeTemp = 0;
    FieldStackItem* current = this->topItem;
    while (current) {
        current = current->below;
        sizeTemp++;
    }
    this->size = sizeTemp;
}

FieldStack::FieldStackItem* FieldStack::findNewBottomItem() const {
    // function should run with 2+ elements within the stack
    // lowest rank => highest priority (=rank of rectangle which is being processed in the field at the bottom of the stack)
    int bottomRank = this->bottomItem->field->getRectangles()->getCurrentId();

    // hladame pocet elementov, ktorych rank je zhodny s rankom fieldu ulozeneho na spodku stacku
    int equalRankedElements = 0;

    FieldStackItem* current = this->bottomItem;
    while (current) {
        if (current->field->getRectangles()->getCurrentId() == bottomRank) {
            equalRankedElements++;
            current = current->upper;
        } else break;
    }

    // horna cela cast poctu prvkov najnizsieho ranku bude zakladom noveho stacku; "borderRank" potom udava pocet prvkov v novom stacku
    int borderRank = (equalRankedElements + 1) / 2;
    int tempRank = 0;

    // iterate through the stack from the bottom item to find the border element
    current = this->bottomItem;
    while (tempRank < borderRank) {
        current = current->upper;
        tempRank++;
    }

    //current vzdy ukazuje na bottomItem povodneho stacku
    return current;
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
