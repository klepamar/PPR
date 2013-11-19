#include "Field.h"
#include "FieldStack.h"
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <stdlib.h>
#include "mpi.h"

using namespace std;

extern bool verbose;
extern bool verboseProcessCommunication;
extern string myPrefix;

Field::Field(Vector2D dimension) : dimX(dimension.getX()), dimY(dimension.getY()) {
    this->perSum = 0;
    this->rects = new RectList(); // create an empty RectList
    this->fieldArray = new int*[this->dimX];
    for (int i = 0; i < this->dimX; i++) {
        fieldArray[i] = new int[this->dimY];
    }
}

Field::Field(const Field& orig) : dimX(orig.dimX), dimY(orig.dimY) {
    perSum = orig.perSum; // copy simple element
    this->rects = new RectList(*(orig.rects)); //call copy-constructor
    this->fieldArray = new int*[this->dimX]; //create a new field
    for (int i = 0; i < this->dimX; i++) {
        fieldArray[i] = new int[this->dimY];
    }
    //copy values stored in the original field
    for (int i = 0; i < dimX; i++) {
        for (int j = 0; j < dimY; j++) {
            this->fieldArray[i][j] = orig.fieldArray[i][j];
        }
    }
}

Field::~Field() {
    for (int i = 0; i < dimX; i++) {
        delete[] fieldArray[i]; // delete array of int
        fieldArray[i] = NULL;
    }
    delete[] fieldArray; // delete array of int*
    fieldArray = NULL;

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
            fieldArray[i][j] = currentElement; // place 0 into object variable 'field' so that it is not undefined or the value itself

            if (fieldArray[i][j] != 0) { // create new rectangle
                Rectangle* rect = new Rectangle(i, j, fieldArray[i][j]);
                rects->append(rect);
            }
        }
        getline(in, s); // get rid of new line character
    }
}

// destruktivni pro horsi reseni
bool improveSolution(Field* &best, Field* &possiblyBetter) {
    if (verbose || verboseProcessCommunication) {
        if (best)
            cout << "Best perimeter sum: " << best->getPerimetrSum() << endl;
        else cout << "Best perimeter sum: " << "UNDEF" << endl;
        if (possiblyBetter)
            cout << "Possibly better perimeter sum: " << possiblyBetter->getPerimetrSum() << endl;
        else cout << "Possibly better perimeter sum: " << "UNDEF" << endl;
    }
    if (possiblyBetter == NULL) {
        return false;
    }

    if (best == NULL || possiblyBetter->getPerimetrSum() < best->getPerimetrSum()) {
        delete best; // clean-up
        best = possiblyBetter;
        possiblyBetter = NULL; // projistotu abych si timhle pointerem nesmazal lepsi reseni
        return true;
    } else {
        delete possiblyBetter;
        possiblyBetter = NULL; // aby pokud ho mazu nekde venku to nehodilo chybu
        return false;
    }
}

bool Field::solveRectShapes(FieldStack* stack, Field* bestSolution) {
    if (verbose) cout << "SolvingRectShapes for rectangle: " << getRectangles()->getCurrent()->toString() << endl;

    Field* newField;
    vector<Vector2D> shapes;

    shapes = this->findRectShapes();

    rects->getCurrent()->setShape(shapes[0]); // use first shape for for rectangle from this field
    for (int i = 1; i < shapes.size(); i++) { // use other shapes for new copy-constructed fields pushed to stack for further solving
        newField = new Field(*this);
        newField->getRectangles()->getCurrent()->setShape(shapes[i]);

        if (bestSolution == NULL || newField->getPerimetrSum() < bestSolution->getPerimetrSum()) { // do not push worse solutions than currently the best
            stack->push(newField);
        }
    }

    return true; // every rectangle has at least one possible shape
}

vector<Vector2D> Field::findRectShapes() {
    //vector<Vector2D> shapes = rects->getCurrent()->getPreparedShapes();
    Rectangle* rect = rects->getCurrent();
    vector<Vector2D> shapes;

    // Vx,y from N . x*y = area & x <= maxHeight & y <= maxWidth
    for (int x = 1; x <= rect->getArea() && x <= this->dimX; x++) {
        for (int y = 1; y <= rect->getArea() && y <= this->dimY; y++) {
            if (x * y == rect->getArea()) {
                shapes.push_back(Vector2D(x, y));
            }
        }
    }

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

bool Field::solveRectPositions(FieldStack* stack, Field* bestSolution) {
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
        
        if (bestSolution == NULL || newField->getPerimetrSum() < bestSolution->getPerimetrSum()) { // do not push worse solutions than currently the best
            stack->push(newField);
        }
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

    int tmp = fieldArray[baseX][baseY];
    fieldArray[baseX][baseY] = 0; // easier checking
    for (int i = top; i <= bottom; i++) {
        for (int j = left; j <= right; j++) { // i, j - jsou všechny možné pozice horního levého rohu takové aby obdélník stále překríval svojí basePos a zároveň aby nepřesahoval z obdélníku ven

            // i,j - every possible position (of top left corner of this (shaped) rectangle
            for (int k = 0; k < shapeX; k++) {
                for (int l = 0; l < shapeY; l++) { // k, l - pozice jednoho políčka obdélníku s danou pozicí (i, j) ktere je nutné testovat zda neprekryvají nenulové pole
                    // check
                    if (/*i + k >= dimX || i + k >= dimY || - neni nutné díky bottom a right zarazce */ fieldArray[i + k][j + l] != 0) { // cover non-zero cell => it is not allowable position
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
    fieldArray[baseX][baseY] = tmp; // restore zeroed basePos

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

void Field::colorField(Rectangle* rect, int color) {
    if (verbose) cout << "ColoringField:" << endl;

    if (rect->hasPosition()) { // true coloring of area

        for (int i = rect->getPosition().getX(); i < rect->getPosition().getX() + rect->getShape().getX(); i++) {
            for (int j = rect->getPosition().getY(); j < rect->getPosition().getY() + rect->getShape().getY(); j++) {
                fieldArray[i][j] = color;
            }
        }

        // add perimeter
        perSum += rect->getPerimeter();

    } else { // just write area
        fieldArray[rect->getBasePosition().getX()][rect->getBasePosition().getY()] = rect->getArea();
    }

    if (verbose) cout << this->toString();
}

void Field::colorField() {
    colorField(rects->getCurrent(), -1 * rects->getCurrentId());
}

/*
 * for borders: http://www.theasciicode.com.ar/extended-ascii-code/box-drawing-character-ascii-code-196.html
 */
string Field::toString(bool includeFieldArray, bool includeRectangleList) const {
    ostringstream ss;

    ss << "<FIELD>" << endl;
    ss << "dimensions: " << Vector2D(dimX, dimY).toDimensionString() << "=" << dimX * dimY << endl;
    ss << "no rectangles: " << rects->getSize() << endl;
    ss << "perimeter sum: " << rects->getPerimeterSum() << endl;

    if (includeFieldArray) {
        // top border
        ss << "┌";
        for (int i = 0; i < dimY - 1; i++) {
            ss << "────┬";
        }
        ss << "────┐" << endl;

        for (int i = 0; i < dimX; i++) {
            ss << "│";
            for (int j = 0; j < dimY; j++) {
                if (fieldArray[i][j]) {
                    if (fieldArray[i][j] < 0) { // is color
                        ss << setw(3) << (char) ((-fieldArray[i][j] - 1) % 26 + 'A') << " │"; // 26 colors then repeat
                    } else { // is area
                        ss << setw(3) << fieldArray[i][j] << " │";
                    }
                } else { // is empty
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
    }

    if (includeRectangleList) {
        ss << rects->toString(); 
    }

    ss << "</FIELD>" << endl;

    return ss.str();
}

string Field::toString() const {
    return toString(true, true);
}

void Field::pack(void *buffer, int bufferSize, int *bufferPos) {
    int dimXcopy = this->dimX; // je const
    int dimYcopy = this->dimY; // je const
    MPI_Pack(&dimXcopy, 1, MPI_INT, buffer, bufferSize, bufferPos, MPI_COMM_WORLD); // dimX
    MPI_Pack(&dimYcopy, 1, MPI_INT, buffer, bufferSize, bufferPos, MPI_COMM_WORLD); // dimY
    this->rects->pack(buffer, bufferSize, bufferPos); // rects
}

Field * Field::unpack(void *buffer, int bufferSize, int *bufferPos) {
    int dimX, dimY;
    Field* field;

    MPI_Unpack(buffer, bufferSize, bufferPos, &dimX, 1, MPI_INT, MPI_COMM_WORLD); // x
    MPI_Unpack(buffer, bufferSize, bufferPos, &dimY, 1, MPI_INT, MPI_COMM_WORLD); // y

    field = new Field(Vector2D(dimX, dimY)); // sestaveni
    field->rects = RectList::unpack(buffer, bufferSize, bufferPos); // rects

    for (int i = 0; i < field->dimX; i++) { // vynulovat fiedlAray protoze nedokazu rict kde nuly a kde cisla, v ostatnich pripadech to kopiruju z jinyho fieldArray takze to udelam v case a*b
        for (int j = 0; j < field->dimY; j++) {
            field->fieldArray[i][j] = 0; // vynulovani
        }
    }

    field->rects->toFirst(); // na zacatek
    while (field->rects->getCurrent() != NULL) { // vsechny projit
        field->colorField(field->rects->getCurrent(), -1 * field->rects->getCurrentId()); // perSum, fieldArray
        field->rects->toNext();
    }
    field->rects->toUnpositioned(); // vratit se na current;

    return field;
}
