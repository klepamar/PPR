#ifndef RECTLIST_H
#define	RECTLIST_H

#include <stdio.h>

#include "Rectangle.h"

/**
 * single-linked simple list of rectangles
 */
class RectList {
private:

    class RectListItem {
    public:
        RectListItem* next;
        Rectangle* rect;
        RectListItem(Rectangle* rectangle);
    };

    RectListItem* back;
    RectListItem* current;

public:
    RectList();
    RectList(const RectList& orig);
    virtual ~RectList();
    void add(Rectangle* rectangle);
    void toNext();
};

#endif	/* RECTLIST_H */

