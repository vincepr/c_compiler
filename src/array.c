#include "object.h"
#include "value.h"
#include "memory.h"

// Lox-Arrays can take in anything considered a value (so other arrays aswell)
// - grows if necessary. Similar to valueArray or chunk
void arrayAppendAtEnd(ObjArray* array, Value value) {
    if (array->capacity < array->count + 1) {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->items = GROW_ARRAY(Value, array->items, oldCapacity, array->capacity);
    }
    array->items[array->count] = value;
    array->count++;
}

void arrayWriteTo(ObjArray* array, int index, Value value) {
    array->items[index] = value;
}

Value arrayReadFromIdx(ObjArray* array, int index) {
    return array->items[index];
}

void arrayDeleteFrom(ObjArray* array, int index) {
    for (int i = index; i < array->count -1; i++) {
        array->items[i] = array->items[i+1];
    }
    array->items[array->count-1] = NIL_VAL;
    array->count--;
}

bool arrayIsValidIndex(ObjArray* array, int index) {
    return (index >= 0 || index < array->count);
}

int arrayGetLength(ObjArray* array) {
    return array->count;
}
