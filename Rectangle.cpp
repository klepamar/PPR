#include "Rectangle.h"
#include "Vector2D.h"
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

using namespace std;

Rectangle::Rectangle(int basePosX, int basePosY, int area)
: basePos(basePosX, basePosY), area(area) {

}

Vector2D Rectangle::getBasePosition() const {
    return basePos;
}

Vector2D Rectangle::getPosition() const {
    return pos;
}

bool Rectangle::hasPosition() const {
    return !pos.isUndefined();
}

void Rectangle::setPosition(Vector2D position) {
    this->pos = position;
}

Vector2D Rectangle::getShape() const {
    return shape;
}

void Rectangle::setShape(Vector2D shape) {
    this->shape = shape;
}

bool Rectangle::hasShape() const {
    return !shape.isUndefined();
}

int Rectangle::getArea() const {
    return area;
}

int Rectangle::getPerimeter() const {
    if (hasShape() == false) {
        return 0;
    } else {
        return 2 * shape.getX() + 2 * shape.getY();
    }
}

string Rectangle::toString() const {
    ostringstream ss;

    ss << "area: " << getArea() << "; " <<
            "basePos: " << getBasePosition().toPointString() << "; " <<
            "pos: " << getPosition().toPointString() << "; " <<
            "shape: " << getShape().toVectorString();
    
    return ss.str();
}