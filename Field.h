#ifndef FIELD_H
#define	FIELD_H

#include "Vector2D.h"
#include "RectList.h"
class FieldStack; // avoid circular dependency
#include <iostream>
#include <string>
#include <vector>

class Field {
private:
    const int dimX; // dimensions of the field
    const int dimY; // dimensions of the field
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
    void fill(std::istream &in);

    // use current rectangle
    bool solveRectShapes(FieldStack* stack);
    std::vector<Vector2D> findRectShapes();
    bool solveRectPositions(FieldStack* stack);
    std::vector<Vector2D> findRectPositions();
    void colorField();

    std::string toString() const;
};

#endif	/* FIELD_H */

