#ifndef RECTLIST_H
#define	RECTLIST_H

#include <stdio.h>

#include "Rectangle.h"
class Rectangle; // instead of include to avoid cycle dependency, no need of Rectangle functions

/**
 * single-linked simple list of rectangles
 * @TODO šel by jako array list protože já vím kolik bude těch rect protože mám zadáno n, bude se to pak trošku snadněji kopírovat asi
 * klepo: urcite by to bolo jednoduchsie, ale ked vacsina funkcii uz bola implementovana...
 */
class RectList {
private:

    class RectListItem {
    public:
        RectListItem* next;
        Rectangle* rect;

        RectListItem(Rectangle* rectangle) {
            this->next = NULL;
            this->rect = rectangle; // call overloaded function of Rectangle operator=
        }
    };

    int size; // number of elements within the list
    int areaSum;
    RectListItem* firstItem; // first element - used when deleting the linked list
    RectListItem* tailItem; // last element onto which are further elements attached
    RectListItem* currentItem; // currently evaluated rectangle

public:
    RectList();
    RectList(const RectList& orig);
    virtual ~RectList();
    RectList& operator=(const RectList & orig);

    bool isEmpty() const;
    bool isAtEnd() const;
    int getSize() const;
    int getAreaSum() const;
    Rectangle* getCurrent() const;
    Rectangle* toNext();

    void append(Rectangle* rectangle);
    int getPerimeterSum();
};

#endif	/* RECTLIST_H */

