#include "Rectangle.h"

Rectangle::Rectangle(int rowPos, int colPos, int area) {
    this->basePos.setXY(rowPos, colPos);
    this->area = area;
}

Rectangle::Rectangle(const Rectangle& orig) {
    throw "Not implemented yet";
}

Rectangle::~Rectangle() {
}

void Rectangle::setShape(Vector2D shape) {
    this->shape = shape;
}

void Rectangle::setPosition(Vector2D position) {
    this->pos = position;
}

bool Rectangle::hasShape() {
    return !shape.isUndefined();
}

int Rectangle::getArea() {
    return area;
}

