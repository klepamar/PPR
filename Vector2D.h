#ifndef VECTOR2D_H
#define	VECTOR2D_H

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

private:
    int x, y;
};

#endif	/* VECTOR2D_H */

