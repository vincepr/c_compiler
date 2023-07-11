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
    object->isMarked = false;
    // insert this obj to the obj-linked list at the head (so at vm.objects):
    object->next = vm.objects;  
    vm.objects = object;


    #ifdef DEBUG_LOG_GC     // log GC-Event:
    if (FLAG_LOG_GC){
        printf("%p allocate %zu for %d\n", (void*)object, size, type);
    }
    #endif

    return object;
}

// helper for ALLOCATE_OBJ macro - constructor
ObjBoundMethod* newBoundMethod(Value receiver, ObjClosure* method) {
    ObjBoundMethod* bound = ALLOCATE_OBJ(ObjBoundMethod, OBJ_BOUND_METHOD);
    bound->receiver = receiver;
    bound->method = method;
    return bound;
}

// helper for ALLOCATE_OBJ macro - constructor for a new Class Object
ObjClass* newClass(ObjString* name) {
    ObjClass* aClass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
    aClass->name = name;
    initTable(&aClass->methods);
    return aClass;
}

// helper for ALLOCATE_OBJ macro- allocates a new ClosureObject that wraps the ObjFunction we put in
ObjClosure* newClosure(ObjFunction* function) {
    // allocate our Arrays that hold Upvalues in use by this closure:
    ObjUpvalue** upvalues = ALLOCATE(ObjUpvalue*, function->upvalueCount);
    for (int i=0; i<function->upvalueCount; i++) {
        upvalues[i] = NULL;     // initialize the whole array as NULL (needed for GC)
    }
    // allocate the Closure (that wraps the function)
    ObjClosure* closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalueCount = function->upvalueCount;
    return closure;
}

// helper for ALLOCATE_OBJ macro - allocates a new Function and initializes its fields.
ObjFunction* newFunction() {
    ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    function->arity = 0;
    function->upvalueCount = 0;
    function->name = NULL;
    initChunk(&function->chunk);
    return function;
}

// helper for ALLOCATE_OBJ macro - allocates instance (runtime) of a class - we pass in the 'parent'Class we build off
ObjInstance* newInstance(ObjClass* pClass) {
    ObjInstance* instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
    instance->pClass = pClass;
    initTable(&instance->fields);
    return instance;
}

// helper for ALLOCATE_OBJ macro - allocates a Native Function (C-Function wrapped and made accessible in lox)
ObjNative* newNative(NativeFn function) {
    ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->function = function;
    return native;
}

// constructor for ObjString - creates a new ObjString on the heap then initializes the fields
static ObjString* allocateString(char* chars, int length, uint32_t hash) {
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;
    push(OBJ_VAL(string));      // we only push it to the stack in case a GC happens next step
    // insert our String into the stringpool-HashTable:
    // - we use it more as a HashSet (we ONLY care about values so we just NIL the value)
    tableSet(&vm.strings, string, NIL_VAL);
    pop();                      // we remove our savety push from the stack
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
    ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
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

// constructor function for upvalues
ObjUpvalue* newUpvalue(Value* slot) {
    ObjUpvalue* upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
    upvalue->closed = NIL_VAL;
    upvalue->location = slot;
    upvalue->next = NULL;
    return upvalue;
}

// helper for printObject()
static void printFunction(ObjFunction* function) {
    if (function->name == NULL) {               // top level uses a function name of NULL
        printf("<script>");
        return;
    }
    printf("<fn %s>", function->name->chars);    // for other functions we just print out the name
}

/* CUSTOM implementations on top of default lox */

// helper for ALLOCATE_OBJ macro - allocates our dynamic list we use as array in lox
ObjArray* newArray() {
    ObjArray* array = ALLOCATE_OBJ(ObjArray, OBJ_ARRAY);
    array->items = NULL;
    array->count = 0;
    array->capacity = 0;
    return array;
}

// helper for printValue() - print functionality for heap allocated datastructures
void printObject(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_BOUND_METHOD:
            printFunction(AS_BOUND_METHOD(value)->method->function);
            break;
        case OBJ_CLASS:
            printf("%s", AS_CLASS(value)->name->chars);
            break;
        case OBJ_CLOSURE:
            printFunction(AS_CLOSURE(value)->function); // bascially we just print the underlying ObjFunction
            break;
        case OBJ_FUNCTION:
            printFunction(AS_FUNCTION(value));
            break;
        case OBJ_INSTANCE:
            printf("%s instance", AS_INSTANCE(value)->pClass->name->chars);
            break;
        case OBJ_NATIVE:
            printf("<native fn>");
            break;
        case OBJ_STRING:
            printf("%s", AS_CSTRING(value));
            break;
        case OBJ_UPVALUE: // this never gets reached, just there to satisfy all cases (prints the resolved var)
            printf("upvalue");
            break;
        case OBJ_ARRAY: {
            printf("array: [");
            ObjArray* array = AS_ARRAY(value);
            for (int i = 0; i < array->count -1; i++) {
                printValue(array->items[i]);
                printf(", ");
            }
            printf("]");
        }
    }
}