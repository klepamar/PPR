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
    for (int i = 0; i < (5 * b + 1); i++) {
        cout << "-"; // top of the field
    }
    cout << endl;
    for (int i = 0; i < a; i++) {
		cout << "|";
        for (int j = 0; j < b; j++) {
            if (field[i][j]) {
				printf ("%4d|",field [i][j]);
            }
            else {
				printf ("    |");
			}
        }
        cout << endl;
    }
    for (int i = 0; i < (5 * b + 1); i++) {
        cout << "-"; // bottom of the field
    }
    cout << endl;
}

void Field::fill(ifstream &in) {
    int currentElement;
    string s,inputElement;
    for (int i = 0; i < a; i++) {
        for (int j = 0; j < b; j++) {
            in >> inputElement;
			currentElement = atoi (inputElement.c_str());	// atoi returns 0 if invalid value found/zero discovered -> invalid value discarded and silently ignored
            field[i][j] = currentElement; // place 0 into object variable 'field' so that it is not undefined or the value itself
        }
        getline(in, s); // get rid of new line character
    }
}
