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

    Vector2D getDimension();
    RectList* getRectangles();
    int getPerimetrSum();
    void showField() const;
    void fill(istream &in);

    bool solveRectShapes(FieldStack &stack);
    int findRectShapes(int rectangleArea, Vector2D* &shapes); // Create and return array of vectors - Possible heights and widths of the rectangle.   

    bool solveRectPoss(FieldStack &stack);
    int findRectPoss(Rectangle* rectangle, Vector2D* &positions);
    void markRect(Rectangle* rectangle);
};

#endif	/* FIELD_H */

