#ifndef RECTLIST_H
#define	RECTLIST_H

#include <stdio.h>

#include "Rectangle.h"
class Rectangle; // instead of include to avoid cycle dependency, no need of Rectangle functions

/**
 * single-linked simple list of rectangles
 * @TODO šel by jako array list protože já vím kolik bude těch rect protože mám zadáno n, bude se to pak trošku snadněji kopírovat asi
 */
class RectList {
private:

    class RectListItem {
    public:
        RectListItem* next;
        Rectangle* rect;
        RectListItem(Rectangle* rectangle);
    };

    int size;
    int areaSum;
    RectListItem* tailItem;
    RectListItem* currentItem;

public:
    RectList();
    RectList(const RectList& orig);
    virtual ~RectList();

    int getSize();
    Rectangle* getCurrent();

    void append(Rectangle* rectangle);
    Rectangle* toNext();
    
    int getAreaSum();
};

#endif	/* RECTLIST_H */

