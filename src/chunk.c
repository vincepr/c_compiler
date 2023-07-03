#include <stdlib.h>
#include "chunk.h"
#include "memory.h"

// initialize a new Chunk. with Default size 0 /empty
void initChunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;    
    chunk->lines = NULL;
    initValueArray(&chunk->constants);
}

// reset the Chunk to its default state of 0 length 
// and deallocate all its previously used space
void freeChunk(Chunk* chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);  // We deallocate all of the memory
    FREE_ARRAY(int, chunk->lines, chunk->capacity);     // free our lines array
    freeValueArray(&chunk->constants);  // we also free our custom pool of constants.
    initChunk(chunk);   // and then zero out the fields -> leaving the chunk in a reset "empty-state"
}

// if we have capacity (pre-allocated space) left we write to it, if not we reallocate with a bigger capacity
void writeChunk(Chunk* chunk, uint8_t byte, int line) {
    if (chunk->capacity < chunk->count + 1) {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
        chunk->lines = GROW_ARRAY(int, chunk->lines, oldCapacity, chunk->capacity);
    }
    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}

// convenient function to add a new constant to the constants-pool
int addConstant(Chunk* chunk, Value value) {
    writeValueArray(&chunk->constants, value);
    return chunk->constants.count - 1;  // returns idx to current last element
}

