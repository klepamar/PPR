#ifndef RECTANGLE_H
#define	RECTANGLE_H

#include "Vector2D.h"

using namespace std;

class Rectangle {
private:
    Vector2D basePos; // position of number that define rectangle - x = row, y = column
    Vector2D pos; // position of left top corner of the rectangle
    int area; // area of rectangle
    Vector2D shape; // x = height, y = width

public:
    Rectangle(int rowPos, int colPos, int volume);
    Rectangle(const Rectangle& orig);
    virtual ~Rectangle();

    void setShape(Vector2D shape);
    void setPosition(Vector2D position);
    void setBasePosition(Vector2D basePosition);
	Vector2D getShape () const;
	Vector2D getPosition () const;
	Vector2D getBasePosition () const;
    bool hasShape();
    int getArea();
    
    Rectangle & operator= (const Rectangle & orig);

};

#endif	/* RECTANGLE_H */

