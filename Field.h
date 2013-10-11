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
    const int dimX, dimY; // dimensions of the field
    int** field; //representation of the field
    RectList* rects; // rectangles - list of rectangles
    int perSum; // perimetrSum - sum of perimeters of all rectangles initially set to 0

public:
    Field(Vector2D dimension);
    Field(const Field& orig);
    virtual ~Field();

    Vector2D getDimension() const; // obtain dimensions of the field
    RectList* getRectangles() const; // obtain a list of rectangles
    int getPerimetrSum() const;
    
    void showField() const;
    void fill(istream &in);

    bool solveRectShapes(FieldStack &stack); // save all combinations of (x,y) such that x.y = area onto the stack
    int findRectShapes(int rectangleArea, Vector2D* &shapes); // create and return array of vectors - possible heights and widths of the rectangle.   

    bool solveRectPoss(FieldStack &stack); // save all combinations of starting position of a rectangle of the given dimensions onto the stack
    int findRectPoss(Rectangle* rectangle, Vector2D* &positions); // find all possible positions of a rectangle of the given dimensions
    void markRect(Rectangle* rectangle);
};

#endif	/* FIELD_H */

