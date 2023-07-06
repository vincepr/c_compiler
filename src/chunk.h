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
    OP_CONSTANT,        // load/produces a static/constant value.

    // OP-instrunctions to push NIL, True, False on the stack
    OP_NIL,
    OP_TRUE,
    OP_FALSE,

    // Stack operations:
    OP_POP,             // pop top value of the stack and disregard it.
    OP_GET_LOCAL,       // read current local value and push it on the stack
    OP_SET_LOCAL,       // writes to local-variable the top value on the stack.(doesnt touch top of stack)
    OP_GET_GLOBAL,      // read current global val  and push it on stack
    OP_DEFINE_GLOBAL,   // define a global variable (initialize it)
    OP_SET_GLOBAL,      // writes to existing global variable

    // euality and comparioson operators ( since  a<=b == !(a>b) these 3 are enough to cover all 6)
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,

    // Binary operators:
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    // unary operators:
    OP_NOT,             // logical Not. like !true -> false
    OP_NEGATE,          // unary negation. like -x with x=3 -> -3
    // special Instructions:
    OP_JUMP,            // always jumps: used to skipp execution of the else case (if..else...)
    OP_JUMP_IF_FALSE,   // used to skipp execution of the statement, for ex:  "if(expr) statement"
    OP_LOOP,            // unconditionally jumps back to the 16-bit offset that follows in 2 8bit chunks afterwards
    OP_CALL,            // a function call
    OP_CLOSURE,         // like OP_CONSTANT but we wrap it in an Closure
    OP_PRINT,           // print expression. like "print x+8;"  -> with x="hello" -> "hello8"
    OP_RETURN,          // return from the current function
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

