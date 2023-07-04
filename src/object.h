#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "chunk.h"
#include "value.h"

/*
    All Heap allocated Lox-Datastructures use this struct.
    - (on The Heap) each Obj starts with a tag field, that identifies what kind of object it is (string, instance, function)
    - afterwards the data field follows.
*/


#define ALLOCATE_OBJ(type, objectType) \
    (type*)allocateObject(sizeof(type), objectType)

// macro to easily access the tag-type
#define OBJ_TYPE(value)     (AS_OBJ(value)->type)

// macro to check if Value is of provided type (need this check to cast it etc.)
#define IS_FUNCTION(value)  isObjType(value, OBJ_FUNCTION)
#define IS_NATIVE(value)    isObjType(value, OBJ_NATIVE)
#define IS_STRING(value)    isObjType(value, OBJ_STRING)

// macros take a Value (that is expected to contain a pointer to a valid ObjString)
#define AS_FUNCTION(value)  ((ObjFunction*)AS_OBJ(value))
#define AS_NATIVE(value)    (((ObjNative*)AS_OBJ(value))->function)
#define AS_STRING(value)    ((ObjString*)AS_OBJ(value))             // this returns the ObjString* pointer
#define AS_CSTRING(value)   (((ObjString*)AS_OBJ(value))->chars)    // this returns the character array itself

// all supported ObjTypes our Language supports
typedef enum {
    OBJ_FUNCTION,
    OBJ_NATIVE,
    OBJ_STRING,
} ObjType;

// The Obj that gets allocated on the stack:
struct Obj {
    ObjType type;       // tag-type (is the following data a string, a function ....)
    struct Obj* next;   // linked list-ish to next Object. This is used to keep track on active heap -> GarbageColleciton
};

// Each Function needs its own Chunk (Callstack, etc...)
typedef struct {
    Obj obj;
    int arity;
    Chunk chunk;
    ObjString* name;
} ObjFunction;

// Native Functions - like print functionality hard coded OR binding of external (here C) OS-functions
typedef Value (*NativeFn)(int argCount, Value* args);
typedef struct {
    Obj obj;
    NativeFn function;
} ObjNative;

// the Payload of the string - this lives on the heap.
struct ObjString {
    Obj obj;
    int length;
    char* chars;
    uint32_t hash;      // we precalculate/hash the hash. (so we dont have to do it each time we use our map)
};

ObjFunction* newFunction();
ObjNative* newNative(NativeFn function);
ObjString* takeString(char* chars, int length);
ObjString* copyString(const char* chars, int length);

// helper for printValue() - print functionality for heap allocated datastructures
void printObject(Value value);

// Checks if the Value provides is typeof the ObjType provided. (ex if val1 is a 'string')
static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}
#endif