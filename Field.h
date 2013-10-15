#ifndef FIELD_H
#define	FIELD_H

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <algorithm>
#include <iomanip>

#include "Vector2D.h"
#include "Rectangle.h"
#include "RectList.h"
#include "FieldStack.h"

using namespace std;

class Field {
private:
    int const dimX; // dimensions of the field
    int const dimY; // dimensions of the field
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
    void fill(istream &in);

    // use current rectangle
    bool solveRectShapes(FieldStack &stack);
    vector<Vector2D> findRectShapes();
    bool solveRectPositions(FieldStack &stack);
    vector<Vector2D> findRectPositions();
    void colorField();

    string toString() const;
};

#endif	/* FIELD_H */

