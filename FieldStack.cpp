#include "FieldStack.h"

FieldStack::FieldStack() {
	this->size = 0;
	this->topItem = this->bottomItem = NULL;
}

FieldStack::FieldStack(const FieldStack& orig) {
}

FieldStack::~FieldStack() {
}

void FieldStack::push(Field* field) {
    // creating a new FieldItem based on the field argument
    FieldStackItem newItem = new FieldStackItem ();
    newItem->field = field;
    newItem->next = newItem->prev = NULL;
    
    if (isEmpty())
    {
		
	}
	else
	{
		
	}
}

Field* FieldStack::pop() {
    throw "Not implemented yet";
}

bool FieldStack::isEmpty() {
	if (this->size == 0) return true;
	return false;
}
