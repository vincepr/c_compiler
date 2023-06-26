#include "common.h"
#include "chunk.h"
#include <stdio.h>

int main(int argc, const char* argv[]) {
    printf("\nCompiler Running !\n\n");
    Chunk chunk;
    initChunk(&chunk);
    writeChunk(&chunk, OP_RETURN);
    freeChunk(&chunk);

    return 0;
}