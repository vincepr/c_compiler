// chunks of instructions: - sequences of bytecode
// these will hold multiple(chunks) assembly instructions
#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

// each instruction has a one-byte operation code - aka opcode
// This number controls what kind of instruction the processor runs, add, subtract, lookup and move value into register ...

// defines all allowed/supported OpCodes:
typedef enum {
    OP_CONSTANT,    // load/produces a static/constant value.
    OP_RETURN,      // return from the current function
} OpCode;

// holds the instructions (dynamic-array of bytes)
typedef struct {
    int count; 
    int capacity;
    uint8_t* code;              // stores op_codes in an array
    int* lines;                 // add info about original line-nr for debugging/for each op_code
    ValueArray constants;       // each chunk of bytecod instructions gets data attached of used static constats etc... (x=4;)
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte, int line);
int addConstant(Chunk* chunk, Value value);

#endif

