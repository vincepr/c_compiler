#ifndef clox_vm_h
#define clox_vm_h

#include "object.h"
#include "chunk.h"
#include "table.h"
#include "value.h"

/*
    The VM - virtual machine takes a chunk of code and runs it.
*/

#define FRAMES_MAX 64                           // max CallFrame depth our vm can handle
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)    // for now the max-stack elements we support is 256


// A CallFrame represents a single ongoing function call. (not returned yet)
typedef struct {
    ObjClosure* closure;            // A pointer to the function beeing called -> we reslove it to its ObjFunction then look that up in our constants-table
    uint8_t* ip;                    // INSTRUCTION POINTER - pointer to the the current instruction-nr were working on in the chunk
    //                                   the 'return address' before the function was called. This is actually the adress of this's CallFrames recent state, when it itself calls a new Function
    Value* slots;                   // points into the VM's value stack at the first slot that this funcion can use
} CallFrame;

// The Instance of our VM - 
typedef struct {
    CallFrame frames[FRAMES_MAX];   // array of CallFrames - The 'call stack' holds reference to all our functions, that havent returned yet. (currently active on the top)
    int frameCount;                 // active instances of CallFrames 
    Value stack[STACK_MAX];         // Stack that holds all currently 'in memory' Values
    Value* stackTop;                // pointer to top of the stack(lastElement + 1) is first to be popped and we add 'above it' when push()
    Table globals;                  // HashMap (key: identifiers, value=global values)
    Table strings;                  // to enable string-interning we store all active-string variables in this table
    
    ObjString* initString;          // for class-initializier init()
    ObjUpvalue* openUpvalues;       // 'linked-list' of Upvalues that currently hold a local-variable they enclosed (that already went out of scope -> now needs to be stored on heap directly)
    Obj* objects;                   // head of the linked list of all objects (strings, instances etc) -> useful for keeping track of active Objects -> GarbageCollection
    // For Garbage-Collection:
    size_t bytesAllocated;          // To keep track of when GC happens we track current Heap-Size and
    size_t nextGC;                  // -> when (at what threshold reached) the next GC should get triggered 
    int grayCount;
    int grayCapacity;
    Obj** grayStack;                // array to keep track of gray-nodes (already visited) but not finished(=black-nodes)
} VM;

// The VM runs the chunk and responds with a value from this enum:
typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern VM vm;       // we expose the vm globally. (since the object-module needs that when allocating a new object (more specific it needs vm.objects <- the linked list))

void initVM();
void freeVM();
InterpretResult interpret(const char* source);
void push(Value value);
Value pop();

#endif
