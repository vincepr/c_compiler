#ifndef clox_value_h
#define clox_value_h

#include "common.h"

// All supported Values of our Language. Like Boolean, Null, double-Number etc...
typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
} ValueType;

// we build one struct that can hold all different kinds of Values our language supports (so we can build an arroy of it that we push pop off)
typedef struct {
    ValueType type;
    // union-type used to save memory used - the size of a union is its largest field. (this case double)
    // it must be taken care that usage of the fields does not overlap (ex a double gets written in and read out as a bool)
    union {
        bool boolean;
        double number;
    } as;       // as is just the name of the union field. Bonus: it reads nice when used to read out the value.
} Value;

// we need macros that convert from our custom Value struct to the C-Values (double) they represent
// converts the Value-struct -> C-values
#define AS_BOOL(value)      ((value).as.boolean)
#define AS_NUMBER(value)    ((value).as.number)
    // there is no nil conversion needed, since it doesnt contain extra data
// converts the C-values -> Value-struct
#define BOOL_VAL(value)     ((Value){VAL_BOOL,   {.boolean = value}})
#define NIL_VAL             ((Value){VAL_NIL,    {.number = 0}})
#define NUMBER_VAL(value)   ((Value){VAL_NUMBER, {.number = value}})
// - we always have to 'typecheck'/know the type of our shared union-'as' field. Or we might read wrong data.
// so we define these macros to 'typecheck'
#define IS_BOOL(value)      ((value).type == VAL_BOOL)
#define IS_NIL(value)       ((value).type == VAL_NIL)
#define IS_NUMBER(value)    ((value).type == VAL_NUMBER)


// pool of Values (Numbers). Uses dynamic array data structure.
// Each chunk of bytecode gets attached one of these pools if it containts Number-data.
typedef struct {
    int capacity;
    int count;
    Value* values;
} ValueArray;

// checks if 2 Values are equal like A==B
bool valuesEqual(Value a, Value b);

// implementing the dynamic data structure:
void initValueArray(ValueArray* array);
void writeValueArray(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);
void printValue(Value value);

#endif