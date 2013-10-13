#ifndef RECTANGLE_H
#define	RECTANGLE_H

#include "Vector2D.h"
#include <vector>

using namespace std;

class Rectangle {
private:
    int const area; // area of rectangle
    Vector2D const basePos; // position of number that define rectangle - x = row, y = column
    Vector2D pos; // position of top left corner of the rectangle
    Vector2D shape; // x = height, y = width
    vector<Vector2D> prepShapes; // prepared shapes from constructor

public:
    Rectangle(int basePositionX, int basePositionY, int volume, int maxHeight, int maxWidth);
    //Rectangle(const Rectangle& orig);
    //virtual ~Rectangle();
    Rectangle& operator= (const Rectangle & orig);

    Vector2D getBasePosition() const;
    Vector2D getPosition() const;
    bool hasPosition() const;
    void setPosition(Vector2D position);
    Vector2D getShape() const;
    void setShape(Vector2D shape);
    bool hasShape() const;
    int getArea() const;
    int getPerimeter() const;
    vector<Vector2D> getPreparedShapes() const;
};

#endif	/* RECTANGLE_H */

