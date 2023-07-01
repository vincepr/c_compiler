#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
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
static ObjString* allocateString(char* chars, int length, uint32_t hash) {
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;
    // insert our String into the stringpool-HashTable:
    // - we use it more as a HashSet (we ONLY care about values so we just NIL the value)
    tableSet(&vm.strings, string, NIL_VAL);     
    return string;
}

// this is our custom Hashing function, that produces our hash value:
static uint32_t hashString(const char* key, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i<length; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

// a bit like copyString() - BUT it takes ownership of the characters you pass in.
//  (this is done so concatenate can just take the input strings and consume them, without extra copying)
ObjString* takeString(char* chars, int length) {
    uint32_t hash = hashString(chars, length);
    ObjString* interned = tablefindString(&vm.strings, chars, length, hash);
    if (interned != NULL) {
        FREE_ARRAY(char, chars, length + 1);    // since it already is in the HashTable we can free the string passed in as args
        return interned;                        // and return reference to the identical string in the stringpool-HashTable
    }
    return allocateString(chars, length, hash);
}

// we take the provided string and allocate it on the heap (leaves passed in chars alone)
// - if the string already exists in our stringpool Hashmap we dont allocate but just return reference to that
ObjString* copyString(const char* chars, int length) {
    uint32_t hash = hashString(chars, length);          // calculate our hash
    ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
    if (interned != NULL) return interned;              // string already exists in stringpool-HashMap so we return reference to it
    
    char* heapChars = ALLOCATE(char, length +1);        // need space for the trailing '\0'
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    return allocateString(heapChars, length, hash);
}

// helper for printValue() - print functionality for heap allocated datastructures
void printObject(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING:
            printf("%s", AS_CSTRING(value));
            break;
    }
}