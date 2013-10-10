#ifndef VECTOR2D_H
#define	VECTOR2D_H

#include <iostream>
#include <climits>
#include <sstream>

using namespace std;

/**
 * Two-dimensional vector.
 * Vector (INT_MIN, INT_MIN) for undefined vector; 
 */
class Vector2D {
public:
    Vector2D();
    Vector2D(int x, int y);
    Vector2D(const Vector2D& orig);
    virtual ~Vector2D();
    void setX(int x);
    int getX() const;
    void setY(int y);
    int getY() const;
    void setXY(int x, int y);
    bool isUndefined();
    int getArea();
    
    string toVectorString() const;
    string toPointString() const;
    string toDimensionString() const;
    friend ostream& operator<<(ostream& os, const Vector2D& vector);


private:
    int x, y;
};

#endif	/* VECTOR2D_H */

