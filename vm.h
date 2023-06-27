#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"

/*
    The VM - virtual machine takes a chunk of code and runs it.
*/

typedef struct {
    Chunk* chunk;
    uint8_t* ip;             // INSTRUCTION POINTER - pointer to the the current instruction-nr were working on in the chunk
    //Value stack[STACK_MAX]; // Stack that holds all currently 'in memory' Values
    //Value* stackTop;        // pointer to top of the stack is first to be popped and we add 'above it' when push()
} VM;

// The VM runs the chunk and responds with a value from this enum:
typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

void initVM();
InterpretResult interpret(Chunk* chunk);
void freeVM();

#endif