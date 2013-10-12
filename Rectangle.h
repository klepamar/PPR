#ifndef RECTANGLE_H
#define	RECTANGLE_H

#include "Vector2D.h"
#include <vector>

using namespace std;

class Rectangle {
private:
    Vector2D basePos; // position of number that define rectangle - x = row, y = column
    Vector2D pos; // position of top left corner of the rectangle
    int area; // area of rectangle
    Vector2D shape; // x = height, y = width
    vector<Vector2D> prepShapes; // prepared shapes from constructor

public:
    Rectangle(int rowPos, int colPos, int volume, int maxHeight, int maxWidth);
    Rectangle(const Rectangle& orig);
    virtual ~Rectangle();

    Vector2D getShape();
    void setShape(Vector2D shape);
    Vector2D getBasePosition();
    Vector2D getPosition();
    void setPosition(Vector2D position);
    void setBasePosition(Vector2D basePosition);
    Vector2D getShape() const;
    Vector2D getPosition() const;
    Vector2D getBasePosition() const;
    bool hasShape();
    int getArea();
    bool hasPosition();
    int getPerimeter();
    vector<Vector2D> getPreparedShapes();
};

#endif	/* RECTANGLE_H */

