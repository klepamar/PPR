#ifndef VECTOR2D_H
#define	VECTOR2D_H

#include <string>

/**
 * Two-dimensional vector.
 * Vector (INT_MIN, INT_MIN) for undefined vector; 
 */
class Vector2D {
private:
    int x, y;

public:
    Vector2D();
    Vector2D(int x, int y);
    void setX(int x);
    int getX() const;
    void setY(int y);
    int getY() const;
    void setXY(int x, int y);
    bool isUndefined() const;
    int getArea() const;
    
    std::string toVectorString() const;
    std::string toPointString() const;
    std::string toDimensionString() const;
    
    void pack(void *outbuffer, int bufferSize, int *bufferPosition);
    static Vector2D unpack(void *inbuffer, int bufferSize, int *bufferPosition); // pozor nevraci pointer ale objekt (vsude s Vector2D pracujeme temer jako s primitivnim typem)
};

#endif	/* VECTOR2D_H */

