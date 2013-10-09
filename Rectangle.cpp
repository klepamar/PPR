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

void Rectangle::setShape(Vector2D shape) {
    this->shape = shape;
}

bool Rectangle::hasShape() {
    return !shape.isUndefined();
}

/**
 * @param shapes in/out pointer to array
 * @return size of array
 */
int Rectangle::getShapes(Vector2D* &shapes) {
    throw "Not implemented yet";
    // všechny dvojice takové že x*y = area && x nepřesahuje počet řádků && y nepřesahuje počet sloupců (řádky sloupce asi řešit až venku tady bych si to musel předávat jinak neznámvelikost fieldu)
}

