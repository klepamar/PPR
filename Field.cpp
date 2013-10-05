#include "Field.h"

Field::Field(int a, int b, int n) {
    this->a = a;
    this->b = b;
    this->n = n;
    this->field = new int*[a];
    for (int i = 0; i < a; i++) {
        field[i] = new int[b];
    }

    // this->rectangles = // nevím jeslti není třeba nějak alokovat tu paměť, vím kolik jich bude tak můžu použít malloc. Ale zatím je nechci inicializovat tak nemůž použít new
    // nebo můzu udělat defaultní konstruktor
}

Field::Field(const Field& orig) {
}

Field::~Field() {
    for (int i = 0; i < a; i++) {
        delete field[a];
    }
}

void Field::showField() const {
    for (int i = 0; i < (2 * b + 1); i++) {
        cout << "-"; // top of the field
    }
    cout << endl;
    for (int i = 0; i < a; i++) {
        for (int j = 0; j < b; j++) {
            if (field[i][j]) {
                cout << "|" << field [i][j]; // non-zero element within the field
            } else {
                cout << "|" << " "; // zero replaced with a blank symbol
            }
        }
        cout << "|" << endl;
    }
    for (int i = 0; i < (2 * b + 1); i++) {
        cout << "-"; // bottom of the field
    }
    cout << endl;
}

void Field::fill(ifstream &in) {
    char currentElement;
    int asciiCode;
    string s;
    for (int i = 0; i < a; i++) {
        for (int j = 0; j < b; j++) {
            in >> currentElement;
            asciiCode = (int) currentElement;
            if (asciiCode == 45) // empty element
            {
                field[i][j] = 0; // place 0 into object variable 'field' so that it is not undefined
            } else if (asciiCode >= 48 && asciiCode <= 57) // is a number
            {
                field[i][j] = asciiCode - 48; // first number stored as char has value of 48 (='0')
            }
        }
        getline(in, s); // get rid of new line character
    }
}
