#ifndef RECTLIST_H
#define	RECTLIST_H

#include <stdio.h>
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
    RectListItem* tail;
    RectListItem* current;

public:
    RectList();
    RectList(const RectList& orig);
    virtual ~RectList();

    int getSize();
    Rectangle* getCurrent();

    void append(Rectangle* rectangle);
    void toNext();
};

#endif	/* RECTLIST_H */

