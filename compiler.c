#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

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
} Precedence;

// used in ParseRule - simple typedef for a function type that takes no arguments and returns nothing.
// used to map TOKEN_ADDITION -> ParseFn implemention for addition.
typedef void (*ParseFn)();

// 
typedef struct {
    ParseFn prefix;             // happens before: this Function implements the parsing function for the Token that maps to that ParseRule
    ParseFn infix;              // happens after : this Function implements the parsing function for the Token that maps to that ParseRule
    Precedence precedence;      // The enum (so actually a Number!) we use to decide what to parse first
} ParseRule;

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

    // Flag that enables dumpink out chunks once the compiler finishes
    #ifdef DEBUG_PRINT_CODE
        if (!parser.hadError) {
            disassembleChunk(currentChunk(), "code");
        }
    #endif
}

// Forward declarations - these get defined later but functions define before and afterwards depend on them. (recursively)
static void expression();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

// parsing function for TOKEN_PLUS, TOKEN_MINUS, TOKEN_START, TOKEN_SLASH
// - when this gets called the left side of the expression has already been parsed and is pop'd on the stack
// - the operand-Symbol is consumed aswell.
// so we just compile the right-side-expression and pop it on the stack, then emit the Bytecode for the Addition.
static void binary() {
    TokenType operatorType = parser.previous.type;
    ParseRule* rule = getRule(operatorType);                // we need to be able to compare precedence to stop at 3 for (2*3+4) and not get the whole 2*7
    parsePrecedence((Precedence)(rule->precedence + 1));

    switch (operatorType) {
        // equality/comparison
        case TOKEN_BANG_EQUAL:      emitBytes(OP_EQUAL, OP_NOT); break;     //(a!=)
        case TOKEN_EQUAL_EQUAL:     emitByte(OP_EQUAL); break;       
        case TOKEN_GREATER:         emitByte(OP_GREATER); break;
        case TOKEN_GREATER_EQUAL:   emitBytes(OP_LESS, OP_NOT); break;      // a>=b == !(a<b)
        case TOKEN_LESS:            emitByte(OP_LESS); break;
        case TOKEN_LESS_EQUAL:      emitBytes(OP_GREATER, OP_NOT); break;   // a<=b == !(a>b)
        // arithmetic
        case TOKEN_PLUS:            emitByte(OP_ADD); break;
        case TOKEN_MINUS:           emitByte(OP_SUBTRACT); break;
        case TOKEN_STAR:            emitByte(OP_MULTIPLY); break;
        case TOKEN_SLASH:           emitByte(OP_DIVIDE); break;
        default: return;            // Unreachable
    }
}

// when hitting a OP_TRUE OP_FALSE OP_NIL we just push the corresponding value on the stack
// this is done as optimisation-strategy (no casting from C-true -> struct and back) 
static void literal() {
    switch (parser.previous.type) {
        case TOKEN_FALSE:       emitByte(OP_FALSE); break;
        case TOKEN_NIL:         emitByte(OP_NIL); break;
        case TOKEN_TRUE:        emitByte(OP_TRUE); break;
        default: return;        // unreachable
    }
}

// parsing function for an expression type - like a recursive descent parser.
static void grouping() {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

// we map TOKEN_NUMBER -> to this function
static void number() {
    double value = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));
}

// parsing function for an unary negation (-10 or !true)
static void unary() {
    TokenType operatorType = parser.previous.type;      // we need to differentiate between ! and -
    parsePrecedence(PREC_UNARY);                        // compiles the operand (ex: 10)

    // Emit the operator instruction (depending on ! or -)
    switch (operatorType) {
        case TOKEN_BANG:        emitByte(OP_NOT); break;
        case TOKEN_MINUS:       emitByte(OP_NEGATE); break;
        default: return;                                // Unreachable
    }
}

// This is just a lookuptable pointing Tokens to the functions that parse it.
// - prefix expressions are on the left side (get evaluated first) then poped on the stack
// - infix expressions are on the right side (get evaluated 2nd) then poped on the stack
// [TOKEN Name]      = {prefix-Fn, infix-Fn, Precedence Number }
ParseRule rules[] = {
  [TOKEN_LEFT_PAREN]    = {grouping, NULL,      PREC_NONE},
  [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,      PREC_NONE},
  [TOKEN_LEFT_BRACE]    = {NULL,     NULL,      PREC_NONE}, 
  [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,      PREC_NONE},
  [TOKEN_COMMA]         = {NULL,     NULL,      PREC_NONE},
  [TOKEN_DOT]           = {NULL,     NULL,      PREC_NONE},
  [TOKEN_MINUS]         = {unary,    binary,    PREC_TERM},
  [TOKEN_PLUS]          = {NULL,     binary,    PREC_TERM},
  [TOKEN_SEMICOLON]     = {NULL,     NULL,      PREC_NONE},
  [TOKEN_SLASH]         = {NULL,     binary,    PREC_FACTOR},
  [TOKEN_STAR]          = {NULL,     binary,    PREC_FACTOR},
  [TOKEN_BANG]          = {unary,    NULL,      PREC_NONE},
  [TOKEN_BANG_EQUAL]    = {NULL,     binary,    PREC_EQUALITY},
  [TOKEN_EQUAL]         = {NULL,     NULL,      PREC_NONE},
  [TOKEN_EQUAL_EQUAL]   = {NULL,     binary,    PREC_EQUALITY},
  [TOKEN_GREATER]       = {NULL,     binary,    PREC_COMPARISON},
  [TOKEN_GREATER_EQUAL] = {NULL,     binary,    PREC_COMPARISON},
  [TOKEN_LESS]          = {NULL,     binary,    PREC_COMPARISON},
  [TOKEN_LESS_EQUAL]    = {NULL,     binary,    PREC_COMPARISON},
  [TOKEN_IDENTIFIER]    = {NULL,     NULL,      PREC_NONE},
  [TOKEN_STRING]        = {NULL,     NULL,      PREC_NONE},
  [TOKEN_NUMBER]        = {number,   NULL,      PREC_NONE},
  [TOKEN_AND]           = {NULL,     NULL,      PREC_NONE},
  [TOKEN_CLASS]         = {NULL,     NULL,      PREC_NONE},
  [TOKEN_ELSE]          = {NULL,     NULL,      PREC_NONE},
  [TOKEN_FALSE]         = {literal,  NULL,      PREC_NONE},
  [TOKEN_FOR]           = {NULL,     NULL,      PREC_NONE},
  [TOKEN_FUN]           = {NULL,     NULL,      PREC_NONE},
  [TOKEN_IF]            = {NULL,     NULL,      PREC_NONE},
  [TOKEN_NIL]           = {literal,  NULL,      PREC_NONE},
  [TOKEN_OR]            = {NULL,     NULL,      PREC_NONE},
  [TOKEN_PRINT]         = {NULL,     NULL,      PREC_NONE},
  [TOKEN_RETURN]        = {NULL,     NULL,      PREC_NONE},
  [TOKEN_SUPER]         = {NULL,     NULL,      PREC_NONE},
  [TOKEN_THIS]          = {NULL,     NULL,      PREC_NONE},
  [TOKEN_TRUE]          = {literal,  NULL,      PREC_NONE},
  [TOKEN_VAR]           = {NULL,     NULL,      PREC_NONE},
  [TOKEN_WHILE]         = {NULL,     NULL,      PREC_NONE},
  [TOKEN_ERROR]         = {NULL,     NULL,      PREC_NONE},
  [TOKEN_EOF]           = {NULL,     NULL,      PREC_NONE},
};

// we need explicit precedence. Otherwise  -a.b + c could be compiled to: - (a.b + c) with how it is written
static void parsePrecedence(Precedence precedence) {
    advance();                                                  // we read the next token
    ParseFn prefixRule = getRule(parser.previous.type)->prefix; // then loop up the corresponding Parse Rule
    // if there is no prefix parser then the token must be a syntax error:
    if (prefixRule == NULL) {
        error("Expect expression.");                            
        return;
    }
    prefixRule();   // otherwise we call the Prefix-ParseFunction
    //                 that call will compile the rest of the prefix expression consuming any other tokens it needs

    // if the next token is too low precedence or isnt an infix operator were done.
    // otherwise  we consume the operand and off control to the infix parser we found ( to get the right side compiled)
    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule();
    }
    
}

// this simply reads the corresponding ParseRule from our rules-lookuptable.
static ParseRule* getRule(TokenType type) {
    return &rules[type];
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