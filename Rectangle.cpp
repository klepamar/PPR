#include "Rectangle.h"

Rectangle::Rectangle(int basePosX, int basePosY, int area, int maxHeight, int maxWidth)
: basePos(basePosX, basePosY), area(area) {
    /*
     * Prepare possible shapes.
     * Vx,y from N . x*y = area & x <= maxHeight & y <= maxWidth
     */
    for (int x = 1; x <= area && x <= maxHeight; x++) {
        for (int y = 1; y <= area && y <= maxWidth; y++) {
            if (x * y == area) {
                this->prepShapes.push_back(Vector2D(x, y));
                cout << "Possible shapes: " << Vector2D(x, y).toVectorString() << endl;
            }
        }
    }
}

/* no need
Rectangle::Rectangle(const Rectangle& orig) {
    this->pos = orig.pos;
    this->basePos = orig.basePos;
    this->shape = orig.shape;
    this->area = orig.area;
}

Rectangle::~Rectangle() {
}
 */

Rectangle& Rectangle::operator=(const Rectangle & orig) {
    cout << "Rect op=";
    /*
        if (this == &orig) // prevent from assignment to itself
            return *this;
        this->pos = orig.pos; // same as copy constructor...
        this->basePos = orig.basePos;
        this->shape = orig.shape;
        this->area = orig.area;

        return *this; // ... but also need to return address of the current object
     */
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

vector<Vector2D> Rectangle::getPreparedShapes() const {
    return prepShapes;
}

string Rectangle::toString() const {
    ostringstream ss;

    ss << "basePos: " << getBasePosition().toPointString() << "; " <<
            "area: " << getArea() << "; " <<
            "prepShapes: ";
    for (int i = 0; i < getPreparedShapes().size(); i++) {
        ss << getPreparedShapes()[i].toVectorString();
        if (i != getPreparedShapes().size() - 1) {
            ss << " ";
        } else {
            ss << "; ";
        }
    }
    ss << "pos: " << getPosition().toPointString() << "; " <<
            "shape: " << getShape().toVectorString();

    return ss.str();
}