#include <iostream>
#include <fstream>
#include <cstdlib> 														//required by EXIT_FAILURE
#include "index.h"

using namespace std;

const char *fileName = "vstup.txt";

Field::Field (int a, int b, int n)
{
	this->a=a;
	this->b=b;
	this->n=n;
	this->field = new int*[a];
	for (int i=0; i<a; i++)
	{
		field[i] = new int[b];	
	}
}
Field::~Field ()
{
	for (int i=0; i<a; i++)
	{
		delete field[a];
	}
}
void Field::showField () const
{
	for (int i=0; i<(2*b+1); i++)
	{
		cout << "-";													// top of the field
	}
	cout << endl;
	for (int i=0; i<a; i++)
	{
		for (int j=0; j<b; j++)
		{
			if (field[i][j])
			{
				cout << "|" << field [i][j];							// non-zero element within the field
			}
			else
			{
				cout << "|" << " ";										// zero replaced with a blank symbol
			}
		}
		cout << "|" << endl;
	}
	for (int i=0; i<(2*b+1); i++)
	{
		cout << "-";													// bottom of the field
	}
	cout << endl;	
}
void Field::fill (ifstream &in)
{
	char currentElement;
	int asciiCode;
	string s;
	for (int i=0; i<a; i++)
	{
		for (int j=0; j<b; j++)
		{
			in >> currentElement;
			asciiCode = (int)currentElement;
			if (asciiCode == 45)										// empty element
			{
				field[i][j]=0;											// place 0 into object variable 'field' so that it is not undefined
			}
			else if (asciiCode >=48 && asciiCode <= 57)					// is a number
			{
				field[i][j]=asciiCode-48;								// first number stored as char has value of 48 (='0')
			}
		}
		getline(in,s); 													// get rid of new line character
	}
}
void readParameters (ifstream &in, int *xDimension, int *yDimension, int *noOfElements)
{
	string s;
	in >> *xDimension >> *yDimension;
	getline(in,s); 														// get rid of new line character
	in >> *noOfElements;
	getline(in,s);	 													// get rid of new line character
	cout << "Dimensions of the field: " << "x=" << *xDimension << ", y=" << *yDimension << endl;
	cout << "Number of non-zero elements: " << *noOfElements << endl;
}
int main (void)
{
	int a,b,n;
    ifstream in;
    in.open (fileName);
    if (!in.is_open())													// opening the file with input parameters
    {
        cerr << "Could not open " << fileName << endl;
        return (EXIT_FAILURE);
    }
    readParameters (in,&a,&b,&n);
    Field myField (a,b,n);
    myField.fill (in);
    myField.showField ();
}
