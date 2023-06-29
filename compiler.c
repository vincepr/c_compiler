#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

/*
    The Compiler implementaiton
    - Parses source Code (Tokens provided by the Scanner)
    - This is a Single-Pass-Compiler -> where in jlox we seperated creating the AST and traverseing it.
        -> here this compiler combines the 2 steps. 
*/

// State of our Parser. We only know about the current and previous Token for context.
typedef struct {
    Token current;
    Token previous;
    
    bool hadError;      // Flag that gets set after parser ran into an error (but we still continue by parsing afterwards)
    bool panicMode;     // Flag helps avoid spewing out 100s of cascading errors, after encountering a first error.
} Parser;

// First has highest Precedence -> it gets executed first
typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,    // =
    PREC_OR,            // or
    PREC_AND,           // and 
    PREC_EQUALITY,      // == !=
    PREC_COMPARISON,    // < <= > >=
    PREC_TERM,          // + -
    PREC_FACTOR,        // * /
    PREC_UNARY,         // ! -
    PREC_CALL,          // . ()
    PREC_PRIMARY,
}

// global Parser instance we can pass arround
Parser parser;
// global Chunk of bytecode-instructions we compile into
Chunk* compilingChunk;

// emits the chunk we compiled our bytecode-instructions to. (so basically all instructions we just 'compiled')
static Chunk* currentChunk() {
    return compilingChunk;
}

// This function logs Errors (so the user can see them)
// - first we print the error ocurred and line were in
// - then we show the lexeme if readable
// - then we print the error message itself.
// - afterwards we set the hadError - FLAG
static void errorAtToken(Token* token, const char* message) {
    if (parser.panicMode) return;       // to avoid spamming 100s of cascading Errors we only report the first error
    parser.panicMode = true;
    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // DO NOTHING (not human readable?)  TODO: check what this would print out?
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }
    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}

// helper               - forwards previousToken and Error-message to errorAtToken()
static void error(const char* message) {
    errorAtToken(&parser.previous, message);
}

// helper for advance() - forwards  currentToken and Error-message to errorAtToken()
static void errorAtCurrent(const char* message) {
    errorAtToken(&parser.current, message);
}

// helper for compile() - keeps advancing till we hit a non-error
static void advance() {
    parser.previous = parser.current;   // save previous-used-Token in parser.previous

    // we keep looping, reading tokens and reporting the errors upstream 
    for (;;) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR) break;
        errorAtCurrent(parser.current.start);
    }
    // untill we hit an non-error then we continue
}

// helper for compile() - reads the next token & advances. 
// - next Type MUST be provided type otherwise ERRORS
static void consume(TokenType type, const char* message) {
    if (parser.current.type= type) {
        advance();
        return;
    }
    errorAtCurrent(message);
}

// after parsing -> translate to a series of bytecoe instructions

// helper - writes given byte and adds it to the chunk of bytecode-instructions
static void emitByte(uint8_t byte) {
    writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

// helper for endCompiler()
static void emitReturn() {
    emitByte(OP_RETURN); // temporaly  - write the OP_RETURN Byte to our Chunk 
}

// helper for emitConstant() - pushes the value on the runtime stack.
static uint8_t makeConstant(Value value) {
    int constant = addConstant(currentChunk(), value);
    if (constant > UINT8_MAX) {
        error("Too many constants in one chunk.");      // at the moment 256 limit of constants per chunk
        return 0;
    }
    return (uint8_t)constant;
}

// emits the Instrucitons to add one constant to our Cunk (like in var x=3.65 -> we would add const 3.65)
static void emitConstant(Value value) {
    emitBytes(OP_CONSTANT, makeConstant(value));
}

// helper for compile() - For now we just add a Return at the end
static void endCompiler() {
    emitReturn();
}

// parsing function for an expression type - like a recursive descent parser.
static void grouping() {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

// we map TOKEN_NUMBER -> to this function
static void number() {
    double value = strtod(parser.previous.start, NULL);
    emitConstant(value);
}

// parsing function for an unary negation (-10 or !true)
static void unary() {
    TokenType operatorType = parser.previous.type;      // we need to differentiate between ! and -
    parsePrecedence(PREC_UNARY)                         // compiles the operand (ex: 10)

    // Emit the operator instruction (depending on ! or -)
    switch (operatorType) {
        case TOKEN_MINUS: emitByte(OP_NEGATE); break;
        default: return;                                // Unreachable
    }
}

// we need explicit precedence. Otherwise  -a.b + c could be compiled to: - (a.b + c) with how it is written
static void parsePrecedence(Precedence precedence) {

}

// helper for compile() 
// - Expressions always evaluate to a Value like 1+2->3 || True == "james"->False 
// - Implemented are so far: 
//      []Number-literals, []parentheses, []unary nengation, []Arithmetics: + - * /
static void expression() {
    parsePrecedence(PREC_ASSIGNMENT);
}

// we pass in the chunk where the compiler will write the code, then try to compile the source
// - if compilation fails we return false (compilation error) to upstread disregard the whole chunk
bool compile(const char* source, Chunk* chunk) {
    initScanner(source);
    compilingChunk = chunk;         // set our current chunk. Compiled bytecode instructions get added to this
    // 'initialize' our Error-FLAGS:
    parser.hadError = false;
    parser.panicMode = false;

    advance();                  // primes the scanner
    expression();
    consume(TOKEN_EOF, "Expected end of expression.");
    endCompiler();
    return !parser.hadError;    // we return if we encountered an compiletime-Error compiling the Chunk
}