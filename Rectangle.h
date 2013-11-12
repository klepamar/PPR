#ifndef RECTANGLE_H
#define	RECTANGLE_H

#include "Vector2D.h"
#include <string>
#include <vector>

class Rectangle {
private:
    int const area; // area of rectangle
    Vector2D const basePos; // position of number that define rectangle - x = row, y = column
    Vector2D pos; // position of top left corner of the rectangle
    Vector2D shape; // x = height, y = width

public:
    Rectangle(int basePositionX, int basePositionY, int area);
    Rectangle(Vector2D basePosition, int area);

    Vector2D getBasePosition() const;
    Vector2D getPosition() const;
    bool hasPosition() const;
    void setPosition(Vector2D position);
    Vector2D getShape() const;
    void setShape(Vector2D shape);
    bool hasShape() const;
    int getArea() const;
    int getPerimeter() const;

    std::string toString() const;

    void pack(void *outbuffer, int bufferSize, int *bufferPosition);
    static Rectangle* unpack(void *inbuffer, int bufferSize, int *bufferPosition);
};

#endif	/* RECTANGLE_H */

