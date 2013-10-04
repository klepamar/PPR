#include <iostream>
#include <fstream>
#include <cstdlib> //required by EXIT_FAILURE
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
	for (int i=0; i<b; i++)
	{
		cout << "_";													//top of the field
	}
	cout << "\n";
	for (int i=0; i<a; i++)
	{
		for (int j=0; j<b; j++)
		{
			cout << "|" << field [i][j];								// element within the field
		}
		cout << "|" << "\n";
	}
	for (int i=0; i<b; i++)
	{
		cout << "_";													//bottom of the field
	}	
}
void readParameters (ifstream &in, int *xDimension, int *yDimension, int *noOfElements)
{
	string s;
	in >> *xDimension >> *yDimension;
	getline(in,s); 														//get rid of new line character
	in >> *noOfElements;
	getline(in,s);	 													//get rid of new line character
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
}
