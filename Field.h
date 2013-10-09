#ifndef FIELD_H
#define	FIELD_H

#include <iostream>
#include <fstream>
#include <stdlib.h>     // for atoi

#include "Vector2D.h"
#include "Rectangle.h"
#include "RectList.h"
#include "FieldStack.h"

using namespace std;

class Field {
private:
    int const dimX, dimY;
    int** field; //representation of the field
    RectList* rects; // rectangles - list of rectangles
    int perSum; // perimetrSum - sum of perimeters of all rectangles

public:
    Field(Vector2D dimension);
    Field(const Field& orig);
    virtual ~Field();

    RectList* getRectangles();
    void showField() const;
    void fill(istream &in);
    
    friend bool operator<(const Field& left, const Field& right);

    bool solveRectShape(FieldStack &stack);
    bool solveRectPos(FieldStack &stack);
    
    
};

#endif	/* FIELD_H */

