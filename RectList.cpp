#include "RectList.h"
#include "Rectangle.h"
#include <cstdlib> 
#include <iostream>
#include <sstream>
#include "mpi.h"

using namespace std;

extern bool verbose;
extern string myPrefix;

RectList::RectList() {
    this->size = 0;
    this->tailItem = NULL;
    this->currentItem = NULL;
    this->headItem = NULL;
}

RectList::RectList(const RectList& orig) {
    this->size = 0;
    this->tailItem = NULL;
    this->currentItem = NULL;
    this->headItem = NULL; // start by creating an empty list

    RectListItem *temp = orig.headItem;
    while (temp != NULL) // ... and continue by appending all items from the original list
    {
        append(new Rectangle(*(temp->rect)));
        if (temp == orig.currentItem) { // this->currentItem have to point to copy of orig->currentItem;
            this->currentItem = this->tailItem;
        }
        temp = temp->next;
    }
    // after this, all pointers and number of rectangles within the list/overall area should be updated automatically (=handled by "append" procedure)
}

RectList::~RectList() {
    RectListItem *temp = headItem;
    RectListItem *nextTemp = headItem;
    while (temp) // use the pointer to the first element in the list
    {
        nextTemp = temp->next;
        delete temp;
        temp = nextTemp;
    }
    this->tailItem = this->currentItem = this->headItem = NULL;
}

bool RectList::isEmpty() const { // this check should correspond to number of "registered" rectangles, i.e. value of "size" variable
    if (tailItem == NULL) return true;
    return false;
}

bool RectList::isAtEnd() const {
    if (currentItem == NULL || currentItem->next == NULL) return true;
    return false;
}

int RectList::getSize() const {
    return this->size;
}

int RectList::getAreaSum() const {
    int sum = 0;

    RectListItem* tmp = this->headItem;
    while (tmp != NULL) {
        sum += tmp->rect->getArea();
        tmp = tmp->next;
    }

    return sum;
}

Rectangle* RectList::getCurrent() const {
    if (currentItem == NULL) {
        return NULL;
    }
    return this->currentItem->rect;
}

int RectList::getCurrentId() const {
    if(currentItem == NULL) {
        return -1; // beyond end
    }
    
    return currentItem->id;
}

void RectList::append(Rectangle* rect) {
    RectListItem* newItem = new RectListItem(rect, size + 1); // create a new item
    if (isEmpty()) {
        headItem = newItem; // remember firstItem so that destructor can properly delete all elements of the list
        currentItem = newItem; // current & tail items will correspond to the only item in the list
    } else {
        tailItem->next = newItem; // append the newly created item at the end of the list
    }
    tailItem = newItem;
    size++;
}

void RectList::toFirst() {
    currentItem = headItem;
}

void RectList::toUnpositioned() {
    toFirst();

    while (currentItem != NULL && currentItem->rect->hasPosition()) { // pokud neni na konci a je uz vyreseny (ma pozici) posunu se na dalsi
        toNext();
    }
}

void RectList::toNext() {
    if (currentItem == NULL) { // we have already visited all items in the list or list is empty
        if (verbose) cout << "Moving to the next rectangle: null" << endl;
    } else {
        currentItem = currentItem->next; // move to next
        if (verbose) cout << "Moving to the next rectangle: " << currentItem->rect->toString() << endl;
    }
}

/**
 * @return 0 for empty RectList, negative integer for incomplete sum and positive number for complete sum
 */
int RectList::getPerimeterSum() const {
    int sum = 0;

    RectListItem* rectItem = headItem;
    while (rectItem != NULL) {
        sum += rectItem->rect->getPerimeter();
        rectItem = rectItem->next;
    }

    if (!isAtEnd()) {
        sum = (-1) * sum; // it is only partial sum 
    }
    return sum;
}

string RectList::toString() const {
    ostringstream ss;

    if (isEmpty()) {
        ss << "<RECTLIST>" << endl << "empty" << endl << "</RECTLIST>" << endl;
        return ss.str();
    }

    ss << "<RECTLIST>" << endl <<
            "size: " << size << endl <<
            "currentItemId: " << getCurrentId() << endl;
    RectListItem* rectItem = headItem;
    while (rectItem != NULL) {
        if (rectItem == headItem) {
            ss << "H";
        } else {
            ss << " ";
        }
        if (rectItem == currentItem) {
            ss << "C";
        } else {
            ss << " ";
        }
        if (rectItem == tailItem) {
            ss << "T";
        } else {
            ss << " ";
        }
        ss << " " << rectItem->rect->toString() << endl;
        rectItem = rectItem->next;
    }
    ss << "</RECTLIST>" << endl;
    return ss.str();
}

void RectList::pack(void *buffer, int bufferSize, int *bufferPos) {

    MPI_Pack(&(this->size), 1, MPI_INT, buffer, bufferSize, bufferPos, MPI_COMM_WORLD); // size
    RectListItem *tmp = this->headItem; // jdu od zacatku abych mohl v unpack pridavat pomoci append
    while (tmp != NULL) {
        tmp->rect->pack(buffer, bufferSize, bufferPos);
        tmp = tmp->next;
    }
}

RectList* RectList::unpack(void *buffer, int bufferSize, int *bufferPos) {
    int size;
    RectList* rectList;
    Rectangle* rect;

    MPI_Unpack(buffer, bufferSize, bufferPos, &size, 1, MPI_INT, MPI_COMM_WORLD); // size
    
    rectList = new RectList(); // sestaveni
    for (int i = 0; i < size; i++) {
        rect = Rectangle::unpack(buffer, bufferSize, bufferPos);
        rectList->append(rect); // headItem, tailItem, rectItems
    }
    
    rectList->toUnpositioned(); // currentItem je vzdy prvni bez pozice

    return rectList;
}