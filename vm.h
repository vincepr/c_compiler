#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "table.h"
#include "value.h"

/*
    The VM - virtual machine takes a chunk of code and runs it.
*/

#define STACK_MAX 256       // for now the max-stack elements we support is 256

typedef struct {
    Chunk* chunk;
    uint8_t* ip;            // INSTRUCTION POINTER - pointer to the the current instruction-nr were working on in the chunk
    Value stack[STACK_MAX]; // Stack that holds all currently 'in memory' Values
    Value* stackTop;        // pointer to top of the stack(lastElement + 1) is first to be popped and we add 'above it' when push()
    Table strings           // to enable string-interning we store all active-string variables in this table
    Obj* objects;           // head of the linked list of all objects (strings, instances etc) -> useful for keeping track of active Objects -> GarbageCollection
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