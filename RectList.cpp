#include "RectList.h"
#include "Rectangle.h"

RectList::RectListItem::RectListItem(Rectangle* rectangle) {
    this->next = NULL;
    this->rect = rectangle;
}

RectList::RectList() {
    this->size = 0;
    this->areaSum = 0;
    this->tailItem = NULL;
    this->currentItem = NULL;
}

RectList::~RectList() {
}

int RectList::getSize() {
    return size;
}

Rectangle* RectList::getCurrent() {
    return currentItem->rect;
}

void RectList::append(Rectangle* rect) {
    if (tailItem == NULL) {
        currentItem = tailItem = new RectListItem(rect);
    } else {
        tailItem->next = new RectListItem(rect);
    }
    areaSum += rect->getArea();
    size++;
}

Rectangle* RectList::toNext() {
    currentItem = currentItem->next;
    return currentItem;
}

int RectList::getAreaSum() {
    return areaSum;
}

