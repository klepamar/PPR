#include "Field.h"

Field::Field(Vector2D dimension) : dimX(dimension.getX()), dimY(dimension.getY()) {
    this->perSum = 0;
    this->rects = new RectList(); // create an empty RectList
    this->field = new int*[dimX];
    for (int i = 0; i < dimX; i++) {
        field[i] = new int[dimY];
    }
}

Field::Field(const Field& orig) : dimX(this->dimX), dimY(this->dimY), perSum(this->perSum) {
    //copy all simple elements
    this->rects = orig.rects; //call overloaded RectList operator=
    
    //create a new field
    this->field = new int*[dimX];
    for (int i = 0; i < dimX; i++) {
        field[i] = new int[dimY];
    }
    //copy values stored in the original field
    for (int i = 0; i < dimX; i++){
		for (int j = 0; j < dimY; j++){
			this->field[i][j] = orig.field[i][j];
		}
	}
}

Field::~Field() {
    for (int i = 0; i < dimX; i++) {
        delete field[dimX];
    }
    delete field;

    delete rects;
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

// for borders: http://www.theasciicode.com.ar/extended-ascii-code/box-drawing-character-ascii-code-196.html

void Field::showField() const {

    cout << "dimensions: " << dimX << "x" << dimY << endl;
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
        if (i != dimX - 1) {
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
                rects->append(new Rectangle(i, j, field[i][j]));
            }
        }
        getline(in, s); // get rid of new line character
    }
}

/**
 * 
 * @return Return false if current rectangle has a shape <=> the rectangle is not resolved (some position left to be tested).
 */
bool Field::solveRectShapes(FieldStack &stack) {
    Field* newField;
    Vector2D* shapes;

    int size = this->findRectShapes(rects->getCurrent()->getArea(), shapes);

    rects->getCurrent()->setShape(shapes[0]); // use first shape for for rectangle from this field
    for (int i = 1; i < size; i++) { // use other shapes for new copy-constructed fields pushed to stack for further solving
        newField = new Field(*this);
        newField->getRectangles()->getCurrent()->setShape(shapes[i]);
        stack.push(newField);
    }

    return true;
}

/**
 * Find all possible shapes for given area in that field.
 * @param rectArea area of rectangle
 * @param shapes in/out pointer to array of founded shapes
 * @return size of array
 */
int Field::findRectShapes(int rectArea, Vector2D* &shapes) {
    throw "Not implemented yet";
    // všechny dvojice takové že x*y = area && x nepřesahuje počet řádků && y nepřesahuje počet sloupců
    // šlo by static pole a ukládat si (jeho velikost je max a*b) a počítat jenom pokuid už jsem to nevypočítal
}

bool Field::solveRectPoss(FieldStack &stack) {
    Field* newField;
    Vector2D* poss;

    int size = this->findRectPoss(rects->getCurrent(), poss);

    if (size == 0) { 
        return false; // no possible position
    }

    rects->getCurrent()->setPosition(poss[0]); // use first position for rectangle from this field
    this->markRect(rects->getCurrent()); // write down the rectangle
    for (int i = 1; i < size; i++) { // use other positions for new copy-constructed fields pushed to stack for further solving
        newField = new Field(*this);
        newField->getRectangles()->getCurrent()->setPosition(poss[i]);
        this->markRect(newField); // write down the rectangle
        stack.push(newField);
    }

    return true;
}

int Field::findRectPoss(Rectangle* rectangle, Vector2D* &positions) {
    throw "Not implemented yet";

    // dva fory
    // najít horní dolní levou pravou zarážku
    // rovnou testovat jestli je tato pozice přípustná (takovyto rect prekryva jen nuly)
}

void Field::markRect(Rectangle* rect) {
    throw "Not implemented yet";

    // vybarvit, přičíst obvod, posunout (nastavit rect jako vyřešenej)
}
