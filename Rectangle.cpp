#include "Rectangle.h"

Rectangle::Rectangle(int rowPos, int colPos, int area) {
    this->basePos.setXY(rowPos, colPos);
    this->area = area;
}

Rectangle::Rectangle(const Rectangle& orig) {
    this->pos = orig.pos;
    this->basePos = orig.basePos;
    this->shape = orig.shape;
    this->area = orig.area;
}

Rectangle::~Rectangle() {
	// no need to use an explicit destructor; all parameters are passed as values
}

void Rectangle::setShape(Vector2D shape) {
    this->shape = shape;
}

void Rectangle::setPosition(Vector2D position) {
    this->pos = position;
}

void Rectangle::setBasePosition(Vector2D basePosition) {
    this->basePos = basePosition;
}

Vector2D Rectangle::getPosition () const
{
	return this->pos;
}

Vector2D Rectangle::getBasePosition () const
{
	return this->basePos;
}

Vector2D Rectangle::getShape() const
{
	return this->shape;
}

bool Rectangle::hasShape() {
    return !shape.isUndefined();
}

int Rectangle::getArea() {
    return area;
}

