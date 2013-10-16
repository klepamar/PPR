#ifndef RECTLIST_H
#define	RECTLIST_H

#include "Rectangle.h"
#include <cstdlib> 
#include <string>

/**
 * single-linked simple list of rectangles
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

        virtual ~RectListItem() {
            delete rect;
        }
    };

    int size; // number of elements within the list
    int areaSum;
    RectListItem* headItem; // first element - used when deleting the linked list
    RectListItem* tailItem; // last element onto which are further elements attached
    RectListItem* currentItem; // currently evaluated rectangle
    int currentItemId;

public:
    RectList();
    RectList(const RectList& orig);
    virtual ~RectList();

    bool isEmpty() const;
    bool isAtEnd() const;
    int getSize() const;
    int getAreaSum() const;
    Rectangle* getCurrent() const;
    int getCurrentId() const;
    void toNext();

    void append(Rectangle* rectangle);
    int getPerimeterSum() const;

    std::string toString() const;
};

#endif	/* RECTLIST_H */

