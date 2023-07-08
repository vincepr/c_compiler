#include <stdlib.h>

#include "compiler.h"
#include "memory.h"
#include "vm.h"


#ifdef DEBUG_LOG_GC // FLAG_LOG_GC
#include <stdio.h>
#include "debug.h"
#endif

#define GC_HEAP_GROW_FACTOR 2           // double threshold when next GC gets triggered each time.

//  The single function used for all dynamic memory management in clox 
//  (this is neccessary for the Garbage Collector)
    // if   -oldSize-   -newSize-       -then do Operation:-
    //      0           Non-zero        Allocate new block-
    //      Non-Zero    0               Free allocation.
    //      Non-Zero    new<oldSize     Shrink existing allocation.
    //      Non-Zero    new>oldSize     Grow existing allocation.
void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
    vm.bytesAllocated += newSize - oldSize;
    if (newSize > oldSize) {
        #ifdef DEBUG_STRESS_GC          // this FLAG -> GC at every possible time
        collectGarbage();
        #endif
        if (vm.bytesAllocated > vm.nextGC) {    
            collectGarbage();           // GC only triggers if the threshold(nextGC) gets exceded
        }
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
    if (object->isMarked) return;       // already fully visited this node
    #ifdef DEBUG_LOG_GC                 // Log GC-Event
    if (FLAG_LOG_GC) { 
        printf("%p mark ", (void*)object);
        printfValue(OBJ_VAL(object));
        printf("\n");
    }
    #endif
    object->isMarked = true;
    // We selfmange this Stack to keep track of gray(already found) nodes
    if (vm.grayCapacity < vm.grayCount +1) {
        vm.grayCapacity = GROW_CAPACITY(vm.grayCapacity);
        // Note how it calls realloc directly (and not reallocate()-wrapper since we dont want to mix into GC):
        vm.grayStack = (Obj**)realloc(vm.grayStack, sizeof(Obj*) * vm.grayCapacity);
        if (vm.grayStack == NULL) exit(1);  // our grayStack ran out of memory -> we cant Continue so we crash-'gracefully'
    }
    vm.grayStack[vm.grayCount++] = object;
}

// For GC - we check if it is actually a heap allocated Obj (stack values like numbers, booleans need no GC)
// if so we pass it down to markObj
void markValue(Value value) {
    if (IS_OBJ(value)) markObject(AS_OBJ(value));
}

// helper for blackenObject - Functions have a table full of Locals etc. that we need to trace
static void markArray(ValueArray* array) {
    for (int i=0; i<array->count; i++) {
        markValue(array->values[i]);
    }
}

// helper for traceReferences() - traverse a single objects references (and mark those/gray them)
static void blackenObject(Obj* object) {
    #ifdef DEBUG_LOG_GC                                     // Log the GC-Event
    if (FLAG_LOG_GC) {
        printf("%p blacken ", (void*)object);
        printValue(OBJ_VAL(object));
        printf("\n");
    }
    #endif

    switch (object->type) {
        case OBJ_CLOSURE: {
            ObjClosure* closure = (ObjClosure*)object;
            markObject((Obj*)closure->function);            // the wrapped function
            for (int i=0; i<closure->upvalueCount; i++) {   // and all the upvalues
                markObject((Obj*)closure->upvalues[i]);
            }
            break;
        }
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*)object;
            markObject((Obj*)function->name);
            markArray(&function->chunk.constants);          // Functions have a table full of Locals etc
            break;
        }
        case OBJ_UPVALUE:
            markValue(((ObjUpvalue*)object)->closed);
            break;
        // contain no outgoing refernces:
        case OBJ_NATIVE:
        case OBJ_STRING:
            break;
    }
}

// infos for --> realloc(void *ptr, size_t size) <--
    // ptr: is the pointer to a memory block previously allocated with malloc/calloc/realloc to be reallocated
        // if it is NULL, a new block is allocated and a pointer to it is returned to by the funciton
    // size: is the new size for the memory block in bytes
    // return value: this function returns a pointer to the newly allocated memory, or NULL if the request fails.


// helper for freeObjects() - frees a single object (node of the linked list)
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
    size_t before = vm.bytesAllocated;
    #endif

    markRoots();                        // starts GC by finding & marking all roots(directly reachable objects by VM)
    traceReferences();                  // walk trough our grayStack will no more grays left (-> we visited everything)
    tableRemoveWhite(&vm.strings);      // we have to specially handle the weak-reference stringpool.
    sweep();                            // now we can cleanup everything not marked
    vm.nextGC = vm.bytesAllocated * GC_HEAP_GROW_FACTOR;    // threshold when next GC gets triggered.

    #ifdef DEBUG_LOG_GC
    if (FLAG_LOG_GC){
        printf("-- GC has ended\n");
        printf("   collected %zu bytes (from %zu to %zu) next at %zu\n", before - vm.bytesAllocated, before, vm.bytesAllocated, vm.nextGC);
    }
    #endif
}

// called when freeVm() shuts our programm down - walk our linked-list of active objects and free each from memory.
void freeObjects() {
    Obj* object = vm.objects;
    while (object != NULL) {
        Obj* next = object->next;
        freeObject(object);
        object = next;
    }
    free(vm.grayStack);
}

// helper for collectGarbage() - starts GC by finding & marking all roots(directly reachable objects by VM)
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

// helper for collectGarbage() - while grayStack isnt empty keep going:
//      1. pick a gray object. Turn any white objects that it holds reference to gray.
//      2. mark the object from previous step black.

static void traceReferences() {
    while (vm.grayCount > 0) {
        Obj* object = vm.grayStack[--vm.grayCount];
        blackenObject(object);          // mark the object black
    }
}

// helper for collectGarbage() - after marking process has finished, this cleans up everything without a mark
// - walks linked-list of every object in the heap, checking their marked bits
static void sweep() {
    Obj* previous = NULL;
    Obj* object = vm.objects;
    while (object != NULL) {
        if (object->isMarked) {
            // has mark -> skipp it
            object->isMarked = false;   // reset state for next GC-cycle
            previous = object;
            object = object->next;
        } else {
            // has NO mark -> free it after looking up next obj. We also connect linked-list from previous->next.
            Obj* unreached = object;
            object = object->next;
            if (previous != NULL) {
                previous->next = object;
            } else {
                vm.objects = object;
            }
            freeObject(unreached);
        }
    }
}