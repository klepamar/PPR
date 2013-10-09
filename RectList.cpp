#include "RectList.h"

RectList::RectListItem::RectListItem(Rectangle* rectangle) {
    this->next = NULL;
    this->rect = rectangle;
}

RectList::RectList() {
    this->size = 0;
    this->tail = NULL;
    this->current = NULL;
}

RectList::~RectList() {
}

int RectList::getSize() {
    return size;
}

void RectList::add(Rectangle* rectangle) {
    if (tail == NULL) {
        current = tail = new RectListItem(rectangle);
    } else {
        tail->next = new RectListItem(rectangle);
    }
    size++;
}

void RectList::toNext() {
    current = current->next;
}

