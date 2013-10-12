#include "Rectangle.h"

Rectangle::Rectangle(int rowPos, int colPos, int area, int maxHeight, int maxWidth) {
    this->basePos.setXY(rowPos, colPos);
    this->area = area;

    /*
     * Prepare possible shapes.
     * Vx,y from N . x*y = area & x <= maxHeight & y <= maxWidth
     */
    for (int x = 1; x <= area && x <= maxHeight; x++) {
        for (int y = 1; y <= area && y < maxWidth; y++) {
            if (x * y == area) {
                this->prepShapes.push_back(Vector2D(x, y));
            }
        }
    }
}

Rectangle::Rectangle(const Rectangle& orig) {
    this->pos = orig.pos;
    this->basePos = orig.basePos;
    this->shape = orig.shape;
    this->area = orig.area;
}

Rectangle::~Rectangle() {
}

Vector2D Rectangle::getShape() {
    return shape;
}

void Rectangle::setShape(Vector2D shape) {
    this->shape = shape;
}

Vector2D Rectangle::getBasePosition() {
    return basePos;
}

Vector2D Rectangle::getPosition() {
    return pos;
}

void Rectangle::setPosition(Vector2D position) {
    this->pos = position;
}

int Rectangle::getArea() {
    return area;
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

bool Rectangle::hasPosition() {
    return !pos.isUndefined();
}

int Rectangle::getPerimeter() {
    if (hasShape() == false) {
        return -1;
    } else {
        return 2 * shape.getX() + 2 * shape.getY();
    }
}

vector<Vector2D> Rectangle::getPreparedShapes() {
    return prepShapes;
}

Rectangle & Rectangle::operator= (const Rectangle & orig)
{
    if (this == &orig) // prevent from assignment to itself
        return *this;
    this->pos = orig.pos; // same as copy constructor...
    this->basePos = orig.basePos;
    this->shape = orig.shape;
    this->area = orig.area;
    
    return *this; // ... but also need to return address of the current object
}
