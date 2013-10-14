#include "RectList.h"
#include "Rectangle.h"

RectList::RectList() {
    this->size = 0;
    this->areaSum = 0;
    this->tailItem = NULL;
    this->currentItem = NULL;
    this->headItem = NULL;
}

RectList::RectList(const RectList& orig) {
    this->size = 0;
    this->areaSum = 0;
    this->tailItem = NULL;
    this->currentItem = NULL;
    this->headItem = NULL; // start by creating an empty list

    RectListItem *temp = orig.headItem;
    while (temp) // ... and continue by appending all items from the original list
    {
        append(temp->rect);
        temp = temp->next;
    }
    // after this, all pointers and number of rectangles within the list/overall area should be updated automatically (=handled by "append" procedure)
}

/*
RectList & RectList::operator=(const RectList & orig) {
    if (this == &orig) // prevent from assignment to itself
        return *this;

    this->size = 0;
    this->areaSum = 0;
    this->tailItem = this->currentItem = this->headItem = NULL; // empty list

    RectListItem *temp = orig.headItem;
    while (temp) // append elements from the original list into the new list
    {
        append(temp->rect);
        temp = temp->next;
    }

    return *this; // do not forget to return the address of the current object
}
 */

RectList::~RectList() {
    RectListItem *temp;
    while (currentItem) // use the pointer to the first element in the list
    {
        temp = currentItem;
        currentItem = currentItem->next;
        delete temp;
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
    return this->currentItem->rect;
}

void RectList::append(Rectangle* rect) {
    RectListItem* newItem = new RectListItem(rect); // create a new item
    if (isEmpty()) {
        headItem = newItem; // remember firstItem so that destructor can properly delete all elements of the list
        currentItem = newItem; // current & tail items will correspond to the only item in the list
    } else {
        tailItem->next = newItem; // append the newly created item at the end of the list
    }
    cout << "Appending new item with area: " << rect->getArea() << endl;
    tailItem = newItem;
    areaSum += rect->getArea();
    size++;
}

Rectangle* RectList::toNext() {
    if (isEmpty() || isAtEnd()) {cout << "rectList::toNext() returning null\n"; return NULL;} // we have already visited all items in the list or list is empty
    currentItem = currentItem->next;
    return currentItem->rect;
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
        sum = (-1) * sum; // it is not 
    }
    return sum;
}

