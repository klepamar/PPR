#include "Field.h"

Field::Field(Vector2D dimension) : dimX(dimension.getX()), dimY(dimension.getY()) {
    this->rects = new RectList();
    this->field = new int*[dimX];
    for (int i = 0; i < dimX; i++) {
        field[i] = new int[dimY];
    }
}

Field::Field(const Field& orig) : dimX(this->dimX), dimY(this->dimY) {
    throw "Not implemented yet";
}

Field::~Field() {
    for (int i = 0; i < dimX; i++) {
        delete field[dimX];
    }
    delete field;

    delete rects;
}

RectList* Field::getRectangles() {
    return rects;
}

// for borders: http://www.theasciicode.com.ar/extended-ascii-code/box-drawing-character-ascii-code-196.html

void Field::showField() const {

    cout << "Dimensions: " << dimX << "x" << dimY << endl;
    cout << "no rectangles: " << rects->getSize() << endl;

    // top border
    cout << "┌";
    for (int i = 0; i < dimY - 1; i++) {
        cout << "────┬";
    }
    cout << "────┐" << endl;

    for (int i = 0; i < dimX; i++) {
        cout << "│";
        for (int j = 0; j < dimY; j++) {
            if (field[i][j]) {
                printf(" %2d │", field [i][j]);
            } else {
                printf("    │");
            }
        }
        cout << endl;

        // inner border
        if (i != dimX-1) {
            cout << "├";
            for (int i = 0; i < dimY - 1; i++) {
                cout << "────┼";
            }
            cout << "────┤" << endl;
        }
    }
    
    // bottom border
    cout << "└";
    for (int i = 0; i < dimY - 1; i++) {
        cout << "────┴";
    }
    cout << "────┘" << endl;
}

void Field::fill(istream &in) {
    int currentElement;
    string s, inputElement;

    for (int i = 0; i < dimX; i++) {
        for (int j = 0; j < dimY; j++) {
            in >> inputElement;
            currentElement = atoi(inputElement.c_str()); // atoi returns 0 if invalid value found/zero discovered -> invalid value discarded and silently ignored
            field[i][j] = currentElement; // place 0 into object variable 'field' so that it is not undefined or the value itself

            if (field[i][j] != 0) {
                rects->add(new Rectangle(i, j, field[i][j]));
            }
        }
        getline(in, s); // get rid of new line character
    }
}

/**
 * 
 * @return Return false if current rectangle has a shape <=> the rectangle is not resolved (some position eft to be tested).
 */
bool Field::solveRectShape() {
    throw "Not implemented yet";
}

bool Field::solveRectPos() {
    throw "Not implemented yet";
}