#include "RectList.h"

RectList::RectListItem::RectListItem(Rectangle* rectangle) {
    this->next = NULL;
    this->rect = rectangle;
}

RectList::RectList() {
    this->back = NULL;
    this->current = NULL;
}

RectList::add(Rectangle* rectangle) {
    if (back == NULL) {
        current = back = new RectListItem(rectangle);
    } else {
        back->next = new RectListItem(rectangle);
    }
}

RectList::toNext() {
    current = current->next;
}

