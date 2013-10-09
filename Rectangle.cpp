#include "Rectangle.h"

Rectangle::Rectangle(int rowPos, int colPos, int area) {
    this->basePos.setXY(rowPos, colPos);
    this->pos.setXY(-1, -1); // not defined yet
    this->area = area;
}

Rectangle::Rectangle(const Rectangle& orig) {
    throw "Not implemented yet";
}

Rectangle::~Rectangle() {
}

Vector2D* Rectangle::getShapes() {
    throw "Not implemented yet";
    // všechny dvojice takové že x*y = area
}

