#ifndef clox_array_h
#define clox_array_h

#include "common.h"


void arrayAppendAtEnd(ObjArray* array, Value value);
void arrayWriteTo(ObjArray* array, int index, Value value);
Value arrayReadFromIdx (ObjArray* array, int index); 
void arrayDeleteFrom(ObjArray* array, int index);
bool arrayIsValidIndex(ObjArray* array, int index);

#endif