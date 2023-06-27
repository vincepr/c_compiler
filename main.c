#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"
#include <stdio.h>

int main(int argc, const char* argv[]) {
    printf("Compiler Running:\n");

    // initialize our VM:
    initVM();

    // init our test-dummy chunk:
    Chunk chunk;
    initChunk(&chunk);

    // fake compiled by hand:      -( (1.2+3.4) / 5.6);
      int constant = addConstant(&chunk, 1.2);
    writeChunk(&chunk, OP_CONSTANT, 123);
    writeChunk(&chunk, constant, 123);
    constant = addConstant(&chunk, 3.4);
    writeChunk(&chunk, OP_CONSTANT, 123);
    writeChunk(&chunk, constant, 123);
    writeChunk(&chunk, OP_ADD, 123);
    constant = addConstant(&chunk, 5.6);
    writeChunk(&chunk, OP_CONSTANT, 123);
    writeChunk(&chunk, constant, 123);
    writeChunk(&chunk, OP_DIVIDE, 123);
    writeChunk(&chunk, OP_NEGATE, 123);
    writeChunk(&chunk, OP_RETURN, 123);

    //disassembleChunk(&chunk, "test chunk");
    interpret(&chunk);
    freeVM();
    freeChunk(&chunk);

    return 0;
}