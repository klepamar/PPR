#include "RectList.h"
#include "Rectangle.h"
#include <cstdlib> 
#include <iostream>
#include <sstream>

using namespace std;

RectList::RectList() {
    this->size = 0;
    this->areaSum = 0;
    this->tailItem = NULL;
    this->currentItem = NULL;
    this->currentItemId = 0;
    this->headItem = NULL;
}

RectList::RectList(const RectList& orig) {
    this->size = 0;
    this->areaSum = 0;
    this->tailItem = NULL;
    this->currentItem = NULL;
    this->currentItemId = 0;
    this->headItem = NULL; // start by creating an empty list

    RectListItem *temp = orig.headItem;
    while (temp != NULL) // ... and continue by appending all items from the original list
    {
        append(new Rectangle(*(temp->rect)));
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
    return this->areaSum;
}

Rectangle* RectList::getCurrent() const {
    if (currentItem == NULL) {
        return NULL;
    }
    return this->currentItem->rect;
}

int RectList::getCurrentId() const {
    return currentItemId;
}

void RectList::append(Rectangle* rect) {
    RectListItem* newItem = new RectListItem(rect); // create a new item
    if (isEmpty()) {
        headItem = newItem; // remember firstItem so that destructor can properly delete all elements of the list
        currentItem = newItem; // current & tail items will correspond to the only item in the list
        currentItemId = 1;
    } else {
        tailItem->next = newItem; // append the newly created item at the end of the list
    }
    cout << "Appending new item with area: " << rect->getArea() << endl;
    tailItem = newItem;
    areaSum += rect->getArea();
    size++;
}

void RectList::toNext() {
    if (currentItem == NULL) { // we have already visited all items in the list or list is empty
        return;
    }
    currentItem = currentItem->next; // move to next
    currentItemId++;

    if (currentItem != NULL) { //
        cout << "Moving to the next rectangle with base pos: " << currentItem->rect->getBasePosition().toPointString() << endl;
    } else {
        cout << "Moving to the next rectangle with base pos: null" << endl;
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

    ss << "RECTLIST" << endl;
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
    return ss.str();
}

