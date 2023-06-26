// chunks of instructions: - sequences of bytecode
#ifndef clox_chunk_h
#define clox_chunk_h

// each instruction has a one-byte operation code - aka opcode
// This number controls what kind of instruction the processor runs, add, subtract, lookup and move value into register ...
#include "common.h"
typedef enum {
    OP_RETURN,      // return from the current function
} OpCode;

// holds the instructions (dynamic-array of bytes)
typedef struct {
    int count;      // 
    int capacity;
    uint8_t* code;
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte);

#endif

