#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "chunk.h"
#include "table.h"
#include "value.h"

/*
    All Heap allocated Lox-Datastructures use this struct.
    - (on The Heap) each Obj starts with a tag field, that identifies what kind of object it is (string, instance, function)
    - afterwards the data field follows.
*/


#define ALLOCATE_OBJ(type, objectType) \
    (type*)allocateObject(sizeof(type), objectType)

// macro to easily access the tag-type
#define OBJ_TYPE(value)         (AS_OBJ(value)->type)

// macro to check if Value is of provided type (need this check to cast it etc.)
#define IS_BOUND_METHOD(value)  isObjType(value, OBJ_BOUND_METHOD)
#define IS_CLASS(value)         isObjType(value, OBJ_CLASS)
#define IS_CLOSURE(value)       isObjType(value, OBJ_CLOSURE)
#define IS_FUNCTION(value)      isObjType(value, OBJ_FUNCTION)
#define IS_INSTANCE(value)      isObjType(value, OBJ_INSTANCE)
#define IS_NATIVE(value)        isObjType(value, OBJ_NATIVE)
#define IS_STRING(value)        isObjType(value, OBJ_STRING)
#define IS_ARRAY(value)         isObjType(value, OBJ_ARRAY)

// macros take a Value (that is expected to contain a pointer to a valid ObjString)
#define AS_BOUND_METHOD(value)  ((ObjBoundMethod*)AS_OBJ(value))
#define AS_CLASS(value)         ((ObjClass*)AS_OBJ(value))
#define AS_CLOSURE(value)       ((ObjClosure*)AS_OBJ(value))
#define AS_FUNCTION(value)      ((ObjFunction*)AS_OBJ(value))
#define AS_INSTANCE(value)      ((ObjInstance*)AS_OBJ(value))
#define AS_NATIVE(value)        (((ObjNative*)AS_OBJ(value))->function)
#define AS_STRING(value)        ((ObjString*)AS_OBJ(value))             // this returns the ObjString* pointer
#define AS_CSTRING(value)       (((ObjString*)AS_OBJ(value))->chars)    // this returns the character array itself
#define AS_ARRAY(value)         ((ObjArray*)AS_OBJ(value))

// all supported ObjTypes our Language supports
typedef enum {
    OBJ_BOUND_METHOD,
    OBJ_CLASS,
    OBJ_CLOSURE,
    OBJ_FUNCTION,
    OBJ_INSTANCE,
    OBJ_NATIVE,
    OBJ_STRING,
    OBJ_UPVALUE,

    OBJ_ARRAY,
} ObjType;

// The Obj that gets allocated on the stack:
struct Obj {
    ObjType type;       // tag-type (is the following data a string, a function ....)
    bool isMarked;      // used for GC. (Objs that someone else holds reference to get marked -> not deleted while GC)
    struct Obj* next;   // linked list-ish to next Object. This is used to keep track on active heap -> used for GC
};

// Each Function needs its own Chunk (Callstack, etc...)
typedef struct {
    Obj obj;
    int arity;          // nr of parameters the function takes in
    int upvalueCount;   // keep track of upvalues we use
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
    uint32_t hash;              // we precalculate/hash the hash. (so we dont have to do it each time we use our map)
};

// Runtime representation for upvalues.
typedef struct ObjUpvalue {
    Obj obj;                    // the Object that this upvalue points to (for example the pointer to Obj enclosing the Number 99)
    Value* location;            // points to the closed over variable. (the variable used to store the 99)
    Value closed;               // once closed (value moves from stack by leaving scope) this will store the value itself on the heap.
    struct ObjUpvalue* next;    // linked list ish points to next upvalue (if there is any)
} ObjUpvalue;

// To enable Closures at runtime we wrap every ObjFunction(created at compiletime) in a ObjClosure, 
// - so we can capture run time-state of encompassing Scopes
typedef struct {
    Obj obj;
    ObjFunction* function;
    ObjUpvalue** upvalues;      // pointer to dynamic upvalue array that stores array of pointers of upvalues
    int upvalueCount;           // we count the nr of Upvalues this Closure holds (useful for GC)
} ObjClosure;

// each class declared gets one of these structs assigned
typedef struct {
    Obj obj;
    ObjString* name;            // pointer to our unique identifier/name (ex.: "class Fish {...}")
    Table methods;              // keeps Track of Methods this class includes
} ObjClass;

// at compile time instances of objects get created (with 'new' keyword OR when a method is called)
typedef struct {
    Obj obj;
    ObjClass* pClass;           // pointer to 'parent Class' this is an instance of
    Table fields;               // we need reference of variables that belong to this instance
} ObjInstance;

// Bound methods wrap the receiver and the method-closure together. To enable the "print this.name;" linking to the instance
typedef struct {
    Obj obj;
    Value receiver;             // ObjInstance where the name-value lives.
    ObjClosure* method;         // the method that does the calling
}ObjBoundMethod;

/* Own implementations top of default-lox */

// a array/list type data structure that basically just wraps the dynamic array
typedef struct {
    Obj obj;
    int count;
    int capacity;
    Value* items;               // the array should take different kind of values, just like a JS-Array
} ObjArray;

ObjBoundMethod* newBoundMethod(Value receiver, ObjClosure* method);
ObjClass* newClass(ObjString* name);
ObjClosure* newClosure(ObjFunction* function);
ObjFunction* newFunction();
ObjInstance* newInstance(ObjClass* pClass);
ObjNative* newNative(NativeFn function);
ObjString* takeString(char* chars, int length);
ObjString* copyString(const char* chars, int length);
ObjUpvalue* newUpvalue(Value* slot);

ObjArray* newArray();

// helper for printValue() - print functionality for heap allocated datastructures
void printObject(Value value);

// Checks if the Value provides is typeof the ObjType provided. (ex if val1 is a 'string')
static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}
#endif