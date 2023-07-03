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

// instruction pointing to a local-variable. But since we dont store names for those, all we can do is return the slot-nr for it.
static int byteInstruction(const char* name, Chunk* chunk, int offset) {
    uint8_t slot = chunk->code[offset + 1];
    printf("%-16s %4d\n", name, slot);
    return offset + 2;
}

// 2 byte/16bit uint jump instructions
static int jumpInstruction(const char* name, int sign, Chunk* chunk, int offset) {
    uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
    jump |= chunk->code[offset + 2];
    printf("%-16s %4d -> %d\n", name, offset, offset+3+sign*jump);
    return offset + 3;
}

// 2 bytes instruction. 1st=OPcode 2nd=index_to_pool_of_Constants/Values for that chunk
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
        case OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", chunk, offset);
        // 1 Bit values:
        case OP_NIL:
            return simpleInstruction("OP_NIL", offset);
        case OP_TRUE:
            return simpleInstruction("OP_TRUE", offset);
        case OP_FALSE:
            return simpleInstruction("OP_FALSE", offset);
        // Stack operations
        case OP_POP:
            return simpleInstruction("OP_POP", offset);
        case OP_GET_LOCAL:
            return byteInstruction("OP_GET_LOCAL", chunk, offset);
        case OP_SET_LOCAL:
            return byteInstruction("OP_SET_LOCAL", chunk, offset);
        case OP_GET_GLOBAL:
            return constantInstruction("OP_GET_GLOBAL", chunk, offset);
        case OP_DEFINE_GLOBAL:
            return simpleInstruction("OP_DEFINE_GLOBAL", offset);
        case OP_SET_GLOBAL:
            return constantInstruction("OP_SET_GLOBAL", chunk, offset);
        // Binary Operators - Comparisons:
        case OP_EQUAL:
            return simpleInstruction("OP_EQUAL", offset);
        case OP_GREATER:
            return simpleInstruction("OP_GREATER", offset);
        case OP_LESS:
            return simpleInstruction("OP_LESS", offset);
        // Binary Operators - Arithmetic:
        case OP_ADD:
            return simpleInstruction("OP_ADD", offset);
        case OP_SUBTRACT:
            return simpleInstruction("OP_SUBTRACT", offset);
        case OP_MULTIPLY:
            return simpleInstruction("OP_MULTIPLY", offset);
        case OP_DIVIDE:
            return simpleInstruction("OP_DIVIDE", offset);
        case OP_NOT:
            return simpleInstruction("OP_NOT", offset);
        case OP_NEGATE:
            return simpleInstruction("OP_NEGATE", offset);
        // special Instructions:
        case OP_JUMP:
            return jumpInstruction("OP_JUMP", 1, chunk, offset);
        case OP_JUMP_IF_FALSE:
            return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
        case OP_PRINT:
            return simpleInstruction("OP_PRINT", offset);
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset +1;
    }
}
