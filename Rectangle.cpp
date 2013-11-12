#include "Rectangle.h"
#include "Vector2D.h"
#include "RectList.h"
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include "mpi.h"

using namespace std;

Rectangle::Rectangle(int basePosX, int basePosY, int area)
: basePos(basePosX, basePosY), area(area) {
}

Rectangle::Rectangle(Vector2D basePosition, int area) 
: basePos(basePosition), area(area) {   
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

void Rectangle::pack(void *buffer, int bufferSize, int *bufferPos) {
    int areaCopy = this->area; // protoze je const nemuzu ji tam dat primo
    Vector2D basePosCopy = this->basePos; // protoze je const nemuzu ji tam dat primo
    MPI_Pack(&areaCopy, 1, MPI_INT, buffer, bufferSize, bufferPos, MPI_COMM_WORLD); // area
    basePosCopy.pack(buffer, bufferSize, bufferPos); // basePos
    this->pos.pack(buffer, bufferSize, bufferPos); // pos
    this->shape.pack(buffer, bufferSize, bufferPos); // shape
}

Rectangle* Rectangle::unpack(void *buffer, int bufferSize, int *bufferPos) {
    int area;
    Vector2D basePos, pos, shape;
    
    MPI_Unpack(buffer, bufferSize, bufferPos, &area, 1, MPI_INT, MPI_COMM_WORLD); // area
    basePos = Vector2D::unpack(buffer, bufferSize, bufferPos); // basePos
    pos = Vector2D::unpack(buffer, bufferSize, bufferPos); // pos
    shape = Vector2D::unpack(buffer, bufferSize, bufferPos); // shape
    
    Rectangle* rect = new Rectangle(basePos, area); // sestaveni
    rect->pos = pos;
    rect->shape = shape;
    
    return rect;
}