#ifndef clox_value_h
#define clox_value_h

#include "common.h"

// represents one constant/static value (clox uses floats for numbers like js)
typedef double Value;

// pool of Values (Numbers). Uses dynamic array data structure.
// Each chunk of bytecode gets attached one of these pools if it containts Number-data.
typedef struct {
    int capacity;
    int count;
    Value* values;
} ValueArray;

// implementing the dynamic data structure:
void initValueArray(ValueArray* array);
void writeValueArray(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);
void printValue(Value value);

#endif