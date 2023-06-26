#include <stdlib.h>
#include "chunk.h"
#include "memory.h"

// initialize a new Chunk. with Default size 0 /empty
void initChunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;    
}

//
void freeChunk(Chunk* chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);  // We deallocate all of the memory
    initChunk(chunk);   // and then zero out the fields -> leaving the chunk in a reset "empty-state"
}

// if we have capacity (pre-allocated space) left we write to it, if not we reallocate with a bigger capacity
void writeChunk(Chunk* chunk, uint8_t byte) {
    if (chunk->capacity < chunk->count + 1) {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
    }
    chunk->code[chunk->count] = byte;
    chunk->count++;
}

