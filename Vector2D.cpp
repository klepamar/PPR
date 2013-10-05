#include "Vector2D.h"

Vector2D::Vector2D() {
    this->x = 0;
    this->y = 0;
}

Vector2D::Vector2D(int x, int y) {
    this->x = x;
    this->y = y;
}

Vector2D::Vector2D(const Vector2D& orig) {
}

Vector2D::~Vector2D() {
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

