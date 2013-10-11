#ifndef FIELD_H
#define	FIELD_H

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <algorithm> 

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

    // use current rectangle
    bool solveRectShapes(FieldStack &stack);
    vector<Vector2D> findRectShapes();
    bool solveRectPositions(FieldStack &stack);
    vector<Vector2D> findRectPositions();
    void colorField();
};

#endif	/* FIELD_H */

