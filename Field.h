#ifndef FIELD_H
#define	FIELD_H

#include <iostream>
#include <fstream>
#include <stdlib.h>     // for atoi

#include "RectList.h"

using namespace std;

class Field {
private:
    int const dimX, dimY;
    int** field; //representation of the field
    RectList* rects; // rectangles - list of rectangles
    int perSum; // perimetrSum - sum of perimetres of all rectangles

public:
    Field(Vector2D dimension);
    Field(const Field& orig);
    virtual ~Field();

    RectList* getRectangles();

    void showField() const;
    void fill(istream &in);

    bool solveRectShape();
    bool solveRectPos();
};

#endif	/* FIELD_H */

