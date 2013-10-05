#include "Rectangle.h"

Rectangle::Rectangle(int rowPos, int colPos, int volume) {
    this->origPos = new Vector2D(rowPos, colPos);
    this->volume = volume;
}

Rectangle::Rectangle(const Rectangle& orig) {
}

Rectangle::~Rectangle() {
    delete origPos;
}

Vector2D* Rectangle::getShapes() {
    // všechny dvojice takové že x*y = volume
}

