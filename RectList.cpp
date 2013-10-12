#include "RectList.h"
#include "Rectangle.h"

RectList::RectList() {
    this->size = 0;
    this->areaSum = 0;
    this->tailItem = NULL;
    this->currentItem = NULL;
    this->firstItem = NULL;
}

RectList::RectList(const RectList& orig) {
    this->size = 0;
    this->areaSum = 0;
    this->tailItem = NULL;
    this->currentItem = NULL;
    this->firstItem = NULL; // start by creating an empty list

    RectListItem *temp = orig.firstItem;
    while (temp) // ... and continue by appending all items from the original list
    {
        append(temp->rect);
        temp = temp->next;
    }
    // after this, all pointers and number of rectangles within the list/overall area should be updated automatically (=handled by "append" procedure)
}

RectList & RectList::operator=(const RectList & orig) {
    if (this == &orig) // prevent from assignment to itself
        return *this;

    this->size = 0;
    this->areaSum = 0;
    this->tailItem = this->currentItem = this->firstItem = NULL; // empty list

    RectListItem *temp = orig.firstItem;
    while (temp) // append elements from the original list into the new list
    {
        append(temp->rect);
        temp = temp->next;
    }

    return *this; // do not forget to return the address of the current object
}

RectList::~RectList() {
    RectListItem *temp;
    while (currentItem) // use the pointer to the first element in the list
    {
        temp = currentItem;
        currentItem = currentItem->next;
        delete temp;
    }
    this->tailItem = this->currentItem = this->firstItem = NULL;
}

bool RectList::isEmpty() const { // this check should correspond to number of "registered" rectangles, i.e. value of "size" variable
    if (tailItem) return true;
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
    RectListItem *newItem = new RectListItem(rect); // create a new item
    if (isEmpty()) {
        currentItem = newItem; // current & tail items will correspond to the only item in the list
        tailItem = newItem;
        firstItem = newItem; // remember firstItem so that destructor can properly delete all elements of the list
    } else {
        tailItem->next = newItem; // append the newly created item at the end of the list
        newItem = tailItem; // make the newly appended item tailItem
    }
    areaSum += rect->getArea();
    this->size = this->size + 1;
}

Rectangle* RectList::toNext() {
    if (isEmpty() || isAtEnd()) return NULL; // we have already visited all items in the list or list is empty
    currentItem = currentItem->next;
    return currentItem->rect;
}

int RectList::getPerimeterSum() {
    throw "Not implemented yet";

    // spočítá přes všechny rect, bylo by vhodný si nějak dát vědět že to je/není celkovej součet
}

