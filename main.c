#include "common.h"
#include "chunk.h"
#include "debug.h"
#include <stdio.h>

int main(int argc, const char* argv[]) {
    printf("Compiler Running:\n");

    // init our test-dummy chunk:
    Chunk chunk;
    initChunk(&chunk);

    // fake compile a constant by hand:.
    int constant = addConstant(&chunk, 1.2);
    writeChunk(&chunk, OP_CONSTANT, 123);
    writeChunk(&chunk, constant, 123);

    // hand-compile the exit->return command:
    writeChunk(&chunk, OP_RETURN, 123);
    disassembleChunk(&chunk, "test chunk");
    freeChunk(&chunk);

    return 0;
}