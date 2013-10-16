#include "Field.h"
#include "FieldStack.h"
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <stdlib.h>

using namespace std;

extern bool verbose;

Field::Field(Vector2D dimension) : dimX(dimension.getX()), dimY(dimension.getY()) {
    this->perSum = 0;
    this->rects = new RectList(); // create an empty RectList
    this->field = new int*[this->dimX];
    for (int i = 0; i < this->dimX; i++) {
        field[i] = new int[this->dimY];
    }
}

Field::Field(const Field& orig) : dimX(orig.dimX), dimY(orig.dimY) {
    perSum = orig.perSum; // copy simple element
    this->rects = new RectList(*(orig.rects)); //call copy-constructor
    this->field = new int*[this->dimX]; //create a new field
    for (int i = 0; i < this->dimX; i++) {
        field[i] = new int[this->dimY];
    }
    //copy values stored in the original field
    for (int i = 0; i < dimX; i++) {
        for (int j = 0; j < dimY; j++) {
            this->field[i][j] = orig.field[i][j];
        }
    }
}

Field::~Field() {
    for (int i = 0; i < dimX; i++) {
        delete[] field[i]; // delete array of int
        field[i] = NULL;
    }
    delete[] field; // delete array of int*
    field = NULL;

    delete rects; // call destructor of RectList
    rects = NULL;
}

Vector2D Field::getDimension() const {
    return Vector2D(dimX, dimY);
}

RectList* Field::getRectangles() const {
    return this->rects; // return an array of rectangles
}

int Field::getPerimetrSum() const {
    return this->perSum;
}

void Field::fill(istream &in) {
    int currentElement;
    string s, inputElement;

    for (int i = 0; i < dimX; i++) {
        for (int j = 0; j < dimY; j++) {
            in >> inputElement;
            currentElement = atoi(inputElement.c_str()); // atoi returns 0 if invalid value found/zero discovered -> invalid value discarded and silently ignored
            field[i][j] = currentElement; // place 0 into object variable 'field' so that it is not undefined or the value itself

            if (field[i][j] != 0) { // create new rectangle
                rects->append(new Rectangle(i, j, field[i][j], dimX, dimY));
            }
        }
        getline(in, s); // get rid of new line character
    }
}

bool Field::solveRectShapes(FieldStack &stack) {
    if (verbose) cout << "SolvingRectShapes for rectangle: " << getRectangles()->getCurrent()->toString() << endl;

    Field* newField;
    vector<Vector2D> shapes;

    shapes = this->findRectShapes();

    rects->getCurrent()->setShape(shapes[0]); // use first shape for for rectangle from this field
    for (int i = 1; i < shapes.size(); i++) { // use other shapes for new copy-constructed fields pushed to stack for further solving
        newField = new Field(*this);
        newField->getRectangles()->getCurrent()->setShape(shapes[i]);
        stack.push(newField);
    }

    return true; // every rectangle has at least one possible shape
}

vector<Vector2D> Field::findRectShapes() {
    vector<Vector2D> shapes = rects->getCurrent()->getPreparedShapes();

    if (verbose) {
        cout << "possible shapes: ";
        for (int i = 0; i < shapes.size(); i++) {
            cout << shapes[i].toVectorString();
            if (i != shapes.size() - 1) {
                cout << " ";
            }
        }
        cout << endl;
    }

    return shapes;
}

bool Field::solveRectPositions(FieldStack &stack) {
    if (verbose) cout << "SolvingRectPositions for rectangle: " << getRectangles()->getCurrent()->toString() << endl;

    Field* newField;
    vector<Vector2D> poss;

    poss = this->findRectPositions();

    if (poss.size() == 0) { // no allowable position
        return false;
    }

    this->rects->getCurrent()->setPosition(poss[0]); // use first position for rectangle from this field
    for (int i = 1; i < poss.size(); i++) { // use other positions for new copy-constructed fields pushed to stack for further solving
        newField = new Field(*this);
        newField->getRectangles()->getCurrent()->setPosition(poss[i]);
        stack.push(newField);
    }

    return true;
}

vector<Vector2D> Field::findRectPositions() {
    Rectangle* rect = rects->getCurrent();
    vector<Vector2D> poss;

    // auxiliary variables
    int baseX = rect->getBasePosition().getX();
    int baseY = rect->getBasePosition().getY();
    int posX = rect->getPosition().getX();
    int posY = rect->getPosition().getY();
    int shapeX = rect->getShape().getX();
    int shapeY = rect->getShape().getY();
    bool flag = true;

    // find top, bottom, left, right "blockers" of a top left corner of the shaped rectangle
    int top = max(baseX - shapeX + 1, 0); // levý horní roh může být nejdříve na pozici [0, 0] a zároveň chci aby obsahoval base pozici
    int left = max(baseY - shapeY + 1, 0); // levý horní roh může být nejdříve na pozici [0, 0] a zároveň chci aby obsahoval base pozici
    //                         kolik presahuje za dimenzi z base
    int bottom = baseX - max((baseX + shapeX - 1) - (dimX - 1), 0); // levý horní roh může být nejdál tam kde se "zrzadlí" (odečtu přesahy) jeho pravy spodní roh pokud je polozen na base
    int right = baseY - max((baseY + shapeY - 1) - (dimY - 1), 0); // levý horní roh může být nejdál tam kde se "zrzadlí" (odečtu přesahy) jeho pravy spodní roh pokud je polozen na base

    if (verbose) {
        cout << "min of top-left corner: " << Vector2D(top, left).toPointString() << endl;
        cout << "max of top-left corner: " << Vector2D(bottom, right).toPointString() << endl;
    }

    int tmp = field[baseX][baseY];
    field[baseX][baseY] = 0; // easier checking
    for (int i = top; i <= bottom; i++) {
        for (int j = left; j <= right; j++) { // i, j - jsou všechny možné pozice horního levého rohu takové aby obdélník stále překríval svojí basePos a zároveň aby nepřesahoval z obdélníku ven

            // i,j - every possible position (of top left corner of this (shaped) rectangle
            for (int k = 0; k < shapeX; k++) {
                for (int l = 0; l < shapeY; l++) { // k, l - pozice jednoho políčka obdélníku s danou pozicí (i, j) ktere je nutné testovat zda neprekryvají nenulové pole
                    // check
                    if (/*i + k >= dimX || i + k >= dimY || - neni nutné díky bottom a right zarazce */ field[i + k][j + l] != 0) { // cover non-zero cell => it is not allowable position
                        flag = false;
                        l = shapeY; // break inner for (where l is control variable)
                        k = shapeX; // break outer for (where k is control variable)
                    }
                }
            }
            if (flag == true) { // it is allowable position
                poss.push_back(Vector2D(i, j));
            }
            flag = true; // reset flag
        }
    }
    field[baseX][baseY] = tmp; // restore zeroed basePos

    if (verbose) {
        cout << "possible positions: ";
        for (int i = 0; i < poss.size(); i++) {
            cout << poss[i].toPointString();
            if (i != poss.size() - 1) {
                cout << " ";
            }
        }
        cout << endl;
    }
    return poss;
}

void Field::colorField() {
    if (verbose) cout << "ColoringField:" << endl;

    Rectangle* rect = rects->getCurrent();
    int color = rects->getCurrentId();

    // color field
    for (int i = rect->getPosition().getX(); i < rect->getPosition().getX() + rect->getShape().getX(); i++) {
        for (int j = rect->getPosition().getY(); j < rect->getPosition().getY() + rect->getShape().getY(); j++) {
            field[i][j] = color;
        }
    }
    color++;

    // add perimeter
    perSum += rect->getPerimeter();

    if (verbose) cout << this->toString();
}

/*
 * for borders: http://www.theasciicode.com.ar/extended-ascii-code/box-drawing-character-ascii-code-196.html
 */
string Field::toString() const {
    ostringstream ss;

    ss << "<FIELD>" << endl;
    ss << "dimensions: " << Vector2D(dimX, dimY).toDimensionString() << endl;
    ss << "no rectangles: " << rects->getSize() << endl;
    ss << "perimeter sum: " << rects->getPerimeterSum() << endl;

    // top border
    ss << "┌";
    for (int i = 0; i < dimY - 1; i++) {
        ss << "────┬";
    }
    ss << "────┐" << endl;

    for (int i = 0; i < dimX; i++) {
        ss << "│";
        for (int j = 0; j < dimY; j++) {
            if (field[i][j]) {
                ss << " " << setw(2) << field [i][j] << " │";
            } else {
                ss << "    │";
            }
        }
        ss << endl;

        // inner border
        if (i != dimX - 1) {
            ss << "├";
            for (int i = 0; i < dimY - 1; i++) {
                ss << "────┼";
            }
            ss << "────┤" << endl;
        }
    }

    // bottom border
    ss << "└";
    for (int i = 0; i < dimY - 1; i++) {
        ss << "────┴";
    }
    ss << "────┘" << endl;

    ss << rects->toString(); // Vypisovat i RectList

    ss << "</FIELD>" << endl;

    return ss.str();
}