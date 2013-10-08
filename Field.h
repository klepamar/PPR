#ifndef FIELD_H
#define	FIELD_H

#include <iostream>
#include <fstream>
#include <stdlib.h>     // for atoi

#include "RectList.h"

using namespace std;

class Field {
private:
    int a; //number of lines (rows))
    int b; //number of columns
    int n; //number of non-zero elements
    int** field; //representation of the field
    RectList rectangles; // list of rectangles
    int perimetrSum; // sum of perimetres of all rectangles
public:
    Field(int a, int b, int n);
    Field(const Field& orig);
    virtual ~Field();
    void showField() const;
    void fill(ifstream &in);
};

#endif	/* FIELD_H */

