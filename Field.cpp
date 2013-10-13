#include "Field.h"

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

/*
Field & Field::operator=(const Field & orig) {
    if (this == &orig) // prevent from assignment to itself
        return *this;

    this->dimX = orig.dimX; // copy elements passed by value
    this->dimY = orig.dimY;
    this->perSum = orig.perSum;

    this->rects = orig.rects; // use RectList overloaded operator= function to create RectList*

    this->field = new int*[dimX]; // create int **field
    for (int i = 0; i < dimX; i++) {
        field[i] = new int[dimY];
    }

    for (int i = 0; i < dimX; i++) { // fill original values into int ** field
        for (int j = 0; j < dimY; j++) {
            this->field[i][j] = orig.field[i][j];
        }
    }

    return *this; // do not forget to return address of the object
}
 */

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

/*
 * for borders: http://www.theasciicode.com.ar/extended-ascii-code/box-drawing-character-ascii-code-196.html
 */
void Field::showField() const {

    cout << "dimensions: " << dimX << "x" << dimY << endl;
    cout << "no rectangles: " << rects->getSize() << endl;
    // @TODO vypsat součet obvodu, nějak řešit jestli je to celkovej nebo jen částečnej

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

            if (field[i][j] != 0) { // create new rectangle
                rects->append(new Rectangle(i, j, field[i][j], dimX, dimY));
            }
        }
        getline(in, s); // get rid of new line character
    }
}

bool Field::solveRectShapes(FieldStack &stack) {
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
    return rects->getCurrent()->getPreparedShapes();
}

bool Field::solveRectPositions(FieldStack &stack) {
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

    // find top, bottom, left, right "blockers"
    int top = max(baseX - shapeX + 1, 0);
    int bottom = min(baseX + shapeX - 1, dimX - 1);
    int left = max(baseY - shapeY + 1, 0);
    int right = min(baseY + shapeY - 1, dimY - 1);

    int tmp = field[baseX][baseY];
    field[baseX][baseY] = 0; // easier checking
    for (int i = top; i <= bottom; i++) {
        for (int j = left; j <= right; j++) {

            // i,j - every possible position (of top left corner of this (shaped) rectangle
            for (int k = 0; k < shapeX; k++) {
                for (int l = 0; l < shapeY; l++) {
                    // check
                    if (field[i + k][j + l] != 0) { // cover non-zero cell => it is not allowable position
                        flag = false;
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
    field[baseX][baseY] = tmp;

    return poss;
}

void Field::colorField() {
    Rectangle* rect = rects->getCurrent();
    static int color = 1; // @TODO nebude fungovat pro předávání mezi procesorama

    // color field
    for (int i = rect->getPosition().getX(); i < rect->getShape().getX(); i++) {
        for (int j = rect->getPosition().getY(); j < rect->getShape().getY(); j++) {
            field[i][j] = color;
        }
    }
    color++;

    // add perimeter
    perSum += rect->getPerimeter();
}
