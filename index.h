class Field
{
private:
	int a;			//number of lines
	int b;			//number of columns
	int n;			//number of non-zero elements
	int ** field;	//representation of the field
public:
	Field (int a, int b, int n);
	~Field ();
	void showField () const;
	void fill (std::ifstream &in);
};
