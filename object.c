#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"


// helper for ALLOCATE_OBJ macro - allocates the object on the heap.
static Obj* allocateObject(size_t size, ObjType type) {
    Obj* object = (Obj*)reallocate(NULL, 0, size);
    object->type = type;
    // insert this obj to the obj-linked list at the head (so at vm.objects):
    object->next = vm.objects;  
    vm.objects = object;
    return object;
}

// constructor for ObjString - creates a new ObjString on the heap then initializes the fields
static ObjString* allocateString(char* chars, int length) {
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    return string;
}

// a bit like copyString() - BUT it takes ownership of the characters you pass in.
//  (this is done so concatenate can just take the input strings and consume them, without extra copying)
ObjString* takeString(char* chars, int length) {
    return allocateString(chars, length);
}

// we take the provided string and allocate it on the heap (leaves passed in chars alone)
ObjString* copyString(const char* chars, int length) {
    char* heapChars = ALLOCATE(char, length +1);        // need space for the trailing '\0'
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    return allocateString(heapChars, length);
}

// helper for printValue() - print functionality for heap allocated datastructures
void printObject(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING:
            printf("%s", AS_CSTRING(value));
            break;
    }
}