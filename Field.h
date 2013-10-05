#ifndef FIELD_H
#define	FIELD_H

#include <iostream>
#include <fstream>

#include "Rectangle.h"

using namespace std;

class Field {
private:
    int a; //number of lines
    int b; //number of columns
    int n; //number of non-zero elements
    int** field; //representation of the field
    Rectangle* rectangles; // array of rectangles
    int currentRect; // id of current processed rectangle
public:
    Field(int a, int b, int n);
    Field(const Field& orig);
    virtual ~Field();
    void showField() const;
    void fill(ifstream &in);
};

#endif	/* FIELD_H */

