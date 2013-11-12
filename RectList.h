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
        int id;
        RectListItem* next;
        Rectangle* rect;

        RectListItem(Rectangle* rectangle, int id) {
            this->id = id;
            this->next = NULL;
            this->rect = rectangle;
        }

        virtual ~RectListItem() {
            delete rect;
        }
    };

    int size; // number of elements within the list
    RectListItem* headItem; // first element - used when deleting the linked list
    RectListItem* tailItem; // last element onto which are further elements attached
    RectListItem* currentItem; // currently evaluated rectangle

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
    void toFirst();
    void toUnpositioned();
    void toNext();

    void append(Rectangle* rectangle);
    int getPerimeterSum() const;

    std::string toString() const;

    void pack(void *outbuffer, int bufferSize, int *bufferPosition);
    static RectList* unpack(void *inbuffer, int bufferSize, int *bufferPosition);
};

#endif	/* RECTLIST_H */

