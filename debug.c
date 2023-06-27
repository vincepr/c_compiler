#include <stdio.h>
#include "debug.h"
#include "value.h"

// dis-assemles ALL the instructions in the entire chunk.
// prints out "human readable" / used for debugging/testing

void disassembleChunk(Chunk* chunk, const char* name) {
    printf("== %s ==\n", name);     // first we print out the current chunk.

    for (int offset = 0; offset < chunk->count;) {  // we do not inc offset here
        offset = disassembleInstruction(chunk, offset); // but dissInstruction increments it. This is because instructions might be of different length so we have to check instructions first.
    }
}

/*
// All supported Instructions and what debug-print out they map to #
//  - (also how many bytes big they are) -> how much to increment the offset
*/

// these instructions are exactly 1 byte big (so we just print their name and offset+=1;)
static int simpleInstruction(const char* name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}
// 2 bytes. 1st=OPcode 2nd=index_to_pool_of_Constants/Values for that chunk
static int constantInstruction(const char* name, Chunk* chunk, int offset) {
    uint8_t constant_idx = chunk->code[offset + 1];     // byte after opcode containts idx to constants-pool
    printf("%-16s %4d '", name, constant_idx);
    printValue(chunk->constants.values[constant_idx]);
    printf("'\n");
    return offset + 2;

}


// Reads one byte and tries to match it with known Byte-Code-Instructions
int disassembleInstruction(Chunk* chunk, int offset) {
    printf("%04d ", offset);        // first we print the byte offset of given instruction
    // use the line info:
    if (offset > 0 && chunk->lines[offset] == chunk->lines[offset-1]) {
        printf("  | ");
    } else {
        printf("%4d ", chunk->lines[offset]);
    }


    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        case OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", chunk, offset);
        // Binary Operators:
        case OP_ADD:
            return simpleInstruction("OP_ADD", offset);
        case OP_SUBTRACT:
            return simpleInstruction("OP_SUBTRACT", offset);
        case OP_MULTIPLY:
            return simpleInstruction("OP_MULTIPLY", offset);
        case OP_DIVIDE:
            return simpleInstruction("OP_DIVIDE", offset);
        case OP_NEGATE:
            return simpleInstruction("OP_NEGATE", offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset +1;
    }
}

