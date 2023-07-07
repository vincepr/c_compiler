#include <stdlib.h>

#include "compiler.h"
#include "memory.h"
#include "vm.h"


#ifdef DEBUG_LOG_GC // FLAG_LOG_GC
#include <stdio.h>
#include "debug.h"
#endif

//  The single function used for all dynamic memory management in clox 
//  (this is neccessary for the Garbage Collector)
    // if   -oldSize-   -newSize-       -then do Operation:-
    //      0           Non-zero        Allocate new block-
    //      Non-Zero    0               Free allocation.
    //      Non-Zero    new<oldSize     Shrink existing allocation.
    //      Non-Zero    new>oldSize     Grow existing allocation.
void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
    if (newSize > oldSize) {
        #ifdef DEBUG_STRESS_GC      // this FLAG -> GC at every possible time
        collectGarbage();
        #endif
    }
    if (newSize == 0) {
        free(pointer);
        return NULL;
    }

    void* result = realloc(pointer, newSize);
    if (result == NULL) exit(1);        // We must handle the case of realloc failing (ex. not enough free memory left on OS)
    return result;
}

// For GC - marks Objects as having some reference to it (so it does not get GC'd)
void markObject(Obj* object) {
    if (object == NULL) return;
    #ifdef DEBUG_LOG_GC                 // Log GC-Event
    if (FLAG_LOG_GC) { 
        printf("%p mark ", (void*)object);
        printfValue(OBJ_VAL(object));
        printf("\n");
    }
    #endif
    object->isMarked = true;
}

// For GC - we check if it is actually a heap allocated Obj (stack values like numbers, booleans need no GC)
// if so we pass it down to markObj
void markValue(Value value) {
    if (IS_OBJ(value)) markObject(AS_OBJ(value));
}

// infos for --> realloc(void *ptr, size_t size) <--
    // ptr: is the pointer to a memory block previously allocated with malloc/calloc/realloc to be reallocated
        // if it is NULL, a new block is allocated and a pointer to it is returned to by the funciton
    // size: is the new size for the memory block in bytes
    // return value: this function returns a pointer to the newly allocated memory, or NULL if the request fails.


// heler for freeObjects() - frees a single object (node of the linked list)
static void freeObject(Obj* object) {
    #ifdef DEBUG_LOG_GC                 // log GC-Event
    if (FLAG_LOG_GC){
        printf("%p free type %d\n", (void*)object, object->type);
    }
    #endif

    switch (object->type) {
        case OBJ_CLOSURE: {
            ObjClosure* closure = (ObjClosure*)object;
            FREE_ARRAY(ObjUpvalue*, closure->upvalues, closure->upvalueCount);
            FREE(ObjClosure, object);   // we free only the ObjClosure NOT the ObjFunction
            break;                      // because the closure doesnt own the function (GC will do that)
        }
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*)object;
            freeChunk(&function->chunk);
            FREE(ObjFunction, object);  // functions have to free their own stack
            break;
        }
        case OBJ_NATIVE: {
            FREE(ObjNative, object);
            break;
        }
        case OBJ_STRING: {
            ObjString* string = (ObjString*)object;
            FREE_ARRAY(char, string->chars, string->length + 1);
            FREE(ObjString, object);
            break;
        }
        case OBJ_UPVALUE: 
            FREE(ObjUpvalue, object);   // ObjUpvalue does not own variable -> only free the reference (GC handles rest)
            break;
    }
}

// starts our garbage collecting process
void collectGarbage() {
    #ifdef DEBUG_LOG_GC
    if (FLAG_LOG_GC){
        printf("-- GC begins\n");
    }
    #endif

    markRoots();            // starts GC by finding & marking all roots(directly reachable objects by VM)

    #ifdef DEBUG_LOG_GC
    if (FLAG_LOG_GC){
        printf("-- GC has ended\n");
    }
    #endif
}

// walk our linked-list of active objects and free each from memory.
void freeObjects() {
    Obj* object = vm.objects;
    while (object != NULL) {
        Obj* next = object->next;
        freeObject(object);
        object = next;
    }
}

// starts GC by finding & marking all roots(directly reachable objects by VM)
static void markRoots() {
    // walk all the local variables on the stack:
    for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
        markValue(*slot);
    }
    // walk all the CallFrames:
    for (int i=0; i<vm.frameCount; i++) {
        markObject((Obj*)vm.frames[i].closure);
    }
    // walk all the Upvalue linked-list:
    for (ObjUpvalue* upvalue = vm.openUpvalues; upvalue!=NULL; upvalue=upvalue->next) {
        markObject((Obj*)upvalue);
    }
    // walk all the global variables in use:
    markTable(&vm.globals);
    // if GC starts while were still compiling -> we need to GC the compiler-structs aswell
    markCompilerRoots();

}