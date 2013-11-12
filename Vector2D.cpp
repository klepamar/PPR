#include "Vector2D.h"
#include <iostream>
#include <string>
#include <sstream>
#include <climits>
#include "mpi.h"

using namespace std;

Vector2D::Vector2D() {
    this->x = INT_MIN;
    this->y = INT_MIN;
}

Vector2D::Vector2D(int x, int y) {
    this->x = x;
    this->y = y;
}

void Vector2D::setX(int x) {
    this->x = x;
}

int Vector2D::getX() const {
    return x;
}

void Vector2D::setY(int y) {
    this->y = y;
}

int Vector2D::getY() const {
    return y;
}

void Vector2D::setXY(int x, int y) {
    this->x = x;
    this->y = y;
}

bool Vector2D::isUndefined() const {
    if (x == INT_MIN && y == INT_MIN) {
        return true;
    } else {
        return false;
    }
}

int Vector2D::getArea() const {
    return x*y;
}

string Vector2D::toVectorString() const {
    ostringstream ss;
    if (isUndefined()) {
        ss << "(undef)";
    } else {
        ss << "(" << x << ", " << y << ")";
    }
    return ss.str();
}

string Vector2D::toPointString() const {
    ostringstream ss;
    if (isUndefined()) {
        ss << "[undef]";
    } else {
        ss << "[" << x << ", " << y << "]";
    }
    return ss.str();
}

string Vector2D::toDimensionString() const {
    ostringstream ss;
    if (isUndefined()) {
        ss << "undef";
    } else {
        ss << x << "x" << y;
    }
    return ss.str();
}

void Vector2D::pack(void *buffer, int bufferSize, int *bufferPos) {
    MPI_Pack(&(this->x), 1, MPI_INT, buffer, bufferSize, bufferPos, MPI_COMM_WORLD); // x
    MPI_Pack(&(this->y), 1, MPI_INT, buffer, bufferSize, bufferPos, MPI_COMM_WORLD); // y
}

Vector2D Vector2D::unpack(void *buffer, int bufferSize, int *bufferPos) {
    int x, y;
    
    MPI_Unpack(buffer, bufferSize, bufferPos, &x, 1, MPI_INT, MPI_COMM_WORLD); // x
    MPI_Unpack(buffer, bufferSize, bufferPos, &y, 1, MPI_INT, MPI_COMM_WORLD); // y
    
    return Vector2D(x, y);
}