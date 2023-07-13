#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "memory.h"
#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

/*
    The Compiler implementaiton
    - Parses source Code (Tokens provided by the Scanner)
    - This is a Single-Pass-Compiler -> where in jlox we seperated creating the AST and traverseing it.
        -> here this compiler combines the 2 steps. 

    Parsing grammar Rules for Statements/Declaration:
statement       -> exprStmt
                | forStmt
                | ifStmt
                | printStmt
                | returnStmt
                | whileStmt
                | block;

declaration     -> classDecl
                | funDecl
                | varDecl
                | statement;
*/

/*
*
*       Type-Declarations and Global-Variables.
*
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
    PREC_FACTOR,        // * / %
    PREC_UNARY,         // ! -
    PREC_CALL,          // . ()
    PREC_IDX_ARRAY,
    PREC_PRIMARY,
} Precedence;


// used in ParseRule - simple typedef for a function type that takes no arguments and returns nothing.
// used to map TOKEN_ADDITION -> ParseFn implemention for addition.
typedef void (*ParseFn) (bool canAssign);

// Parse Rules Map from the Token to rules that hold precedence(priority) and functions that execute before and after the Token
typedef struct {
    ParseFn prefix;             // happens before: this Function implements the parsing function for the Token that maps to that ParseRule
    ParseFn infix;              // happens after : this Function implements the parsing function for the Token that maps to that ParseRule
    Precedence precedence;      // The enum (so actually a Number!) we use to decide what to parse first
} ParseRule;

// field of Compiler struct - holds info about one Local Variable
typedef struct {
    Token name;
    int depth;
    bool isCaptured;            // take notice when we need to enclose it when leaving scope (Upvalue took reference to it)
} Local;

// We use Upvalues to resolve in Closures captured outer Variables and reslove them to the memory where the actual x=1 is stored.
typedef struct {
    uint8_t index;              // stores which local slot the upvalue is capturing
    bool isLocal;               // isLocal=true -> is a localVariable || isLocal=false -> its another upvalue were chaining together
} Upvalue;

// Compiler needs to differentiate between 2 states, top level and while in a function-body.
typedef enum {
    TYPE_FUNCTION,              // normal function - returns implicit nil if no return
    TYPE_INITIALIZER,           // init() - the method that gets called when creating new instances MUST always return an INSTANCE
    TYPE_METHOD,                // classes in lox can hold methods
    TYPE_SCRIPT                 // Top level (so we can differentiate it from local scope)
} FunctionType;

// we need this struct to keep track of the current scope and all local variables of that scope
typedef struct Compiler {
    struct Compiler* enclosing; // Each Compiler points back to the Comnpiler for the function that encloses it.
    ObjFunction* function;      // the current function were writing to
    FunctionType type;          // are we parsing top-level or in a function-body currently

    Local locals[UINT8_COUNT];  // Flat arrays of all locals that are in scope during this exact point in the compilation.
    int localCount;             // current count
    Upvalue upvalues[UINT8_COUNT];  // array that stores Upvalues (we use them to link enclosed variables in Closures to the actual memory used)
    int scopeDepth;             // how many {} deep are we
} Compiler;

// we need knowledge (at compile time) about nearest enclosing class. this struct provides that
typedef struct ClassCompiler {
    struct ClassCompiler* enclosing;    // linked list of nested Compiler structs. (lox supports a class in a method in a class...)
    bool hasSuperclass;                 // if we deal with a superclass (inherit from another class) -> we need a local scope -> we have to clean that up
} ClassCompiler;

// global Parser instance we can pass arround
Parser parser;
// global Compiler instance - used to keep track where on the stack local variables are currently
Compiler* current = NULL;
// global ClassCompiler Instance - we need knowledge (at compile time) about nearest enclosing class. this provides
ClassCompiler* currentClass = NULL;

// emits the chunk we compiled our bytecode-instructions to. (so basically all instructions we just 'compiled')
static Chunk* currentChunk() {
    return &current->function->chunk;
}

/*
*
*       Helpers that deal with Tokens (checking them, consuming them etc...)
*
*/

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
    if (parser.current.type == type) {
        advance();
        return;
    }
    errorAtCurrent(message);
}

// helper - if the current token has the given type we return true.
static bool check(TokenType type) {
    return parser.current.type == type;
}

// helper - if the current token has the given type we consume it and return true.
static bool match(TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}


/*
*
*       Helpers that deal with ByteCode Instructions
*
*/

// helper - writes given byte and adds it to the chunk of bytecode-instructions
static void emitByte(uint8_t byte) {
    writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

// helper for whileStatement() - 
// - emits a new loop instruction that unconditionally jumps backwards by a given offset(16bit). 
//      (pushed to stack afterwadrds in 2 8bit chunks)
// - 
static void emitLoop(int loopStart) {
    emitByte(OP_LOOP);
    int offset = currentChunk()->count - loopStart + 2; // +2 is because of OP_JUMP 2 bytes for offset's length
    if (offset > UINT16_MAX) error("Loop body too large.");
    emitByte((offset >> 8) & 0xff);     // the two 8bit of the 16bit int we use to jump
    emitByte(offset & 0xff);
}

// helper for patchJump() - emits the input Jump-Instruction then a 2-byte long offset
// - emits a bytecode instruction and writes a placeholder operand for the jump offset
// - we use 2 bytes for the jump offset -> so 65535 bytes of code is our max jump length.
static int emitJump(uint8_t instruction) {
    emitByte(instruction);
    emitByte(0xff);
    emitByte(0xff);
    return currentChunk()->count -2;
}

// helper for endCompiler() - function returns explicit (a value) or implicit by reaching } -> it returns nil
static void emitReturn() {
    if (current->type == TYPE_INITIALIZER) {
        emitBytes(OP_GET_LOCAL, 0);     // init() always must return a new instance of the class.
        // So we push the zero slot -> that we know contains the instance when handling methods.
    } else {
        emitByte(OP_NIL);               // normal functions return implicit nil
    }
    emitByte(OP_RETURN);                // temporaly  - write the OP_RETURN Byte to our Chunk 
}

// helper - we call this function when we exit a new local scope with "}"...
static void endScope() {
    current->scopeDepth--;
    // remove all Local-Variables that went out of scope:
    while (current->localCount > 0 &&
            current->locals[current->localCount - 1].depth > current->scopeDepth) {
        if (current->locals[current->localCount -1].isCaptured) {
            emitByte(OP_CLOSE_UPVALUE);         // if a upvalue closed over the value it takes ownership of it
        } else {
            emitByte(OP_POP);                   // if not we can remove it forever
        }
        current->localCount--;
    }

}

// helper - we call this function when we enter a new local scope with "{"...
static void beginScope() {
        current->scopeDepth++;
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

// init a new Compiler - we use this every time we need a new Chunk/Call-Stack, in ex.: if we hit a new function
static void initCompiler(Compiler* compiler, FunctionType type) {
    compiler->enclosing = current;          // capture previous current compiler and write it to be this one's-enclosing/'parent'
    compiler->function = NULL;
    compiler->type = type;
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    compiler->function = newFunction();     // create a new ObjFunction -> we compile our code into it's chunk.
    current = compiler;
    if (type != TYPE_SCRIPT) {              // if not a top-scope function we store its function-name (copy because of lifetimes)
        current->function->name = copyString(parser.previous.start, parser.previous.length);
    }

    // compiler uses the 0 slot of locals for internal use:
    // - name "" so no one else can write to it (with how our maps work)
    Local* local = &current->locals[current->localCount++];
    local->depth = 0;
    local->isCaptured = false;

    // compiler sets stack slot zero - Used for special purpose
    if (type != TYPE_FUNCTION) {
        local->name.start = "this";         // were dealing with a method -> this slot holds the this keyword
        local->name.length = 4;             // 
    } else {
        local->name.start = "";             // for function calls this slot ends up holding
        local->name.length = 0;             // the function called later on
    }
}

// emits the Instrucitons to add one constant to our Cunk (like in var x=3.65 -> we would add const 3.65)
static void emitConstant(Value value) {
    emitBytes(OP_CONSTANT, makeConstant(value));
}

// helper for ifStatement() - spaws the placeholder offset for the real offset.
// - goes back into the bytecode and replaces the calculated jump offset. 
//   (ex. now we know how long (how many bytecode-instructions) the doStatment after a if(expression)doStatment; was)
static void patchJump(int offset) {
    //
    int jump = currentChunk()->count - offset -2;
    if (jump > UINT16_MAX) {
        error("Too much code to jump over.");   // max of 65535bytes of code (2 8 bit blocks)
    }
    currentChunk()->code[offset] = (jump >> 8) & 0xff;
    currentChunk()->code[offset +1] = jump & 0xff;
}

// helper for compile() - For now we just add a Return at the end
static ObjFunction* endCompiler() {
    emitReturn();
    ObjFunction* function = current->function;   // grab the pointer to the current function

    // Flag that enables dumping out chunks once the compiler finishes
    #ifdef DEBUG_PRINT_CODE
    if (FLAG_PRINT_CODE) {
        if (!parser.hadError) {
            // user define functions have a name, the toplevel one is NULL:
            disassembleChunk(currentChunk(), function->name != NULL ? function->name->chars : "<script>");
        }
    }
    #endif

    current = current->enclosing;               // put the this one's enclosing/'parent'-compiler as the current, when we close the this recent one
    return function;
}

/*
*
*      Forward declarations 
*       - needed to resolve recursive dependencies among those functions. (because c)
*/

static void expression();
static void statement();                    // recursive with declaration()
static void declaration();                  // recursive with statment()
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

/*
*
*  HELPERS for dealing with variables and identifiers
*
*/

// helper - adds lexeme to constant-table. returns idx to it
static uint8_t identifierConstant(Token* name) {
    return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

// helper for declareVariable - cecks if 2 identifiers are the same. (ex. then a and b point to the same variable )
static bool identifiersEqual(Token* a, Token* b) {
    if (a->length != b->length) return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

// helper - tries to find Idx to local-variable in use
// - loop the list of ccurrently in scope local variables. 
//    - backwards because thats top of our stack. This way inner variables get found first! (like shadowed variables)
// - If identifiers match -> return index to it
static int resolveLocal(Compiler* compiler, Token* name) {
    for (int i = compiler->localCount - 1; i>=0; i--) {     
        Local* local = &compiler->locals[i];
        if (identifiersEqual(name, &local->name)) {
            if (local->depth == -1) {
                error("Can't read local variable in its own initializer.");
            }
            return i;   // found the variable
        }
    }
    return -1;          // -1 is our custom signal, that it is not found -> globals will get checked for it now!
}

// adds a new upvalue (used to resolve Closure-captured variables) to the upvalue-array
static int addUpvalue(Compiler* compiler, uint8_t index, bool isLocal) {
    int upvalueCount = compiler->function->upvalueCount;

    // check we find matching Upvalue we reuse it and return it's index
    for (int i=0; i<upvalueCount; i++) {
        Upvalue* upvalue = &compiler->upvalues[i];
        if (upvalue->index == index && upvalue->isLocal == isLocal) {
            return i;
        }
    }
    if (upvalueCount == UINT8_COUNT) {
        error("Too many closure variables in function.");
        return 0;
    }
    // otherwise we add it at the end of the upvalue-array:
    compiler->upvalues[upvalueCount].isLocal = isLocal;
    compiler->upvalues[upvalueCount].index = index;
    return compiler->function->upvalueCount++;
}

// after failing to reslove a local variable this gets called
// - and will start looking trough the Upvalues
static int resolveUpvalue(Compiler* compiler, Token* name) {
    if (compiler->enclosing == NULL) return -1;                 // its global scope
    int local = resolveLocal(compiler->enclosing, name);
    if (local != -1) {
        compiler->enclosing->locals[local].isCaptured = true;   // we note in the local-variable that an upvalue referenced it
        return addUpvalue(compiler, (uint8_t)local, true);
    }
    // it can recursively chain Upvalue->Upvalue->Upvalue->local x=1
    int upvalue = resolveUpvalue(compiler->enclosing, name);
    if (upvalue != -1) {
        return addUpvalue(compiler, (uint8_t)upvalue, false);
    }
    return -1;
}


// helper - Adds Local variable to our Compiler-Struct that keeps track of active local-variables on the stack.
static void addLocal(Token name) {
    if (current->localCount ==UINT8_COUNT) {
        error("Too many local variables in function.");         // Scope is hard capped by array size
        return;
    }
    Local* local = &current->locals[current->localCount++];     // initialize the next available local(next element in array)
    local->name = name;                                         // stores variables Identity
    local->depth = -1;                                          // -1 WE USE to signal an UNITIALIZED VARIABLE
    local->isCaptured = false;
}

// helper for parseVariable() - take The Identifiert and pass it down
static void declareVariable() {
    if(current->scopeDepth == 0) return;                        // this happens only for locals, so for globals we return early
    Token* name = &parser.previous;

    // We have to manually check if were trying to redeclare a local (NOT ALLOWED): like "var x = 1; var x =2;" only "x=2;"" would be
    for (int i = current->localCount-1; i>=0; i--) {
        Local* local = &current->locals[i];
        if (local->depth != -1 && local->depth < current->scopeDepth) {
            break;
        }
        if (identifiersEqual(name, &local->name)) {
            error("Already a variable with this name in this scope.");
        }
    }

    addLocal(*name);
}


// helper working with variables and identifiers - consumes next Token=IDENTIFIER
static uint8_t parseVariable(const char* errorMessage) {
    consume(TOKEN_IDENTIFIER, errorMessage);
    declareVariable();
    if (current->scopeDepth > 0) return 0;          // if were in a scope its a local-var so we dont need to shove it down the constant-Table
    
    return identifierConstant(&parser.previous);    // were defining a global -> so we shove it down the constant-Lookup-Table
}

// helper for defineVariable - utility to get current scopeDepth
//  - used this way to handle the special case of declaring:         var x=9; { var x = x; }
static void markInitialized() {
    // since Function Declarations use this, we return early if global scope -> able to 'use' that function in itself (recursion):
    if (current->scopeDepth == 0) return;       
    current->locals[current->localCount - 1].depth = current->scopeDepth;
}

// helper for varDeclaration() - 
//  - previously the value of our variable got poped to the stack
//  - so now we can just emit this instruction afterwards -> takes that value and stores it 
static void defineVariable(uint8_t global) {
    if (current->scopeDepth > 0) {
        markInitialized();
        return;                             // we hit a local, no need to do the global thing (value is already on top of the stack)
    }
    emitBytes(OP_DEFINE_GLOBAL, global);    // this would remove the value from the stack then write it to our lookup-Table
}

// helper for call() - compile the arguments (of a funciton-call)  ex: doThings(arg1, 99, "Bond James")
static uint8_t argumentList() {
    uint8_t argCount = 0;
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            expression();
            if(argCount == 255) {
                error("Can't have more than 255 arguments.");
            }
            argCount++;
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
    return argCount;
}


/*
*
*      Parsing logic: Binary, Unary, Literal, Grouping logic
*
*/

// parsing function for logical operator: AND
// at the point this is called the left side of AND is evaluated and on top of stack
//  -> if this is already false we skip everything right of AND with a jump.
//     (since "true AND neverRanFunction()" short circuits - we have to use Jumps)
static void and_(bool canAssign) {
    int endJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);                           // cleanup the evaluated left side expr from stack
    parsePrecedence(PREC_AND);                  // continue to eval right side of AND
    patchJump(endJump);
}

// parsing function for TOKEN_PLUS, TOKEN_MINUS, TOKEN_START, TOKEN_SLASH
// - when this gets called the left side of the expression has already been parsed and is pop'd on the stack
// - the operand-Symbol is consumed aswell.
// so we just compile the right-side-expression and pop it on the stack, then emit the Bytecode for the Addition.
static void binary(bool _canAssign) {
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
        // CUSTOM token:
        case TOKEN_MODULO:          emitByte(OP_MODULO); break;
        default: return;            // Unreachable
    }
}

// when hitting an opening '(' followed by an expression (ex, function call)
static void call(bool canAssign) {
    uint8_t argCount = argumentList();      // compiles all Function Arguments
    emitBytes(OP_CALL, argCount);           // invoke the function, using the argument count as operand
}

// parsing function for dot-syntax as in "SomeClass.someField=true; SomeClass.doSomeMethod();"
static void dot(bool canAssign) {
    consume(TOKEN_IDENTIFIER, "Expect property name after '.'.");
    uint8_t name = identifierConstant(&parser.previous);
    if(canAssign && match(TOKEN_EQUAL)) {
        expression();
        emitBytes(OP_SET_PROPERTY, name);    // "Preaches.isTasty = false" sets isTasty field
    } else if (match(TOKEN_LEFT_PAREN))  {
        uint8_t argCount = argumentList();
        emitBytes(OP_INVOKE, name);         // Method calls get its own OP-Code for optimisation
        emitByte(argCount);
    } else {
        emitBytes(OP_GET_PROPERTY, name);   // "print Peaches.isTasty" -> prints true
    }
}

// when hitting a OP_TRUE OP_FALSE OP_NIL we just push the corresponding value on the stackexpect
// this is done as optimisation-strategy (no casting from C-true -> struct and back) 
static void literal(bool _canAssign) {
    switch (parser.previous.type) {
        case TOKEN_FALSE:       emitByte(OP_FALSE); break;
        case TOKEN_NIL:         emitByte(OP_NIL); break;
        case TOKEN_TRUE:        emitByte(OP_TRUE); break;
        default: return;        // unreachable
    }
}

// parsing function for an expression type - like a recursive descent parser.
static void grouping(bool _canAssign) {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

// we map TOKEN_NUMBER -> to this function
static void number(bool _canAssign) {
    double value = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));
}

// parsing function for logical OR
// - control flowy. In this example the right side is not reached: "false OR isNotReached()" -> we use Jumps
// NOTE: this could be made in a new OP_CODE skipp -> less instructions needed -> faster
static void or_(bool _canAssign) {
    int elseJump = emitJump(OP_JUMP_IF_FALSE);  // if left-side is falsey -> tiny jump over next statement
    int endJump = emitJump(OP_JUMP);            // (else) if left side is true -> big jump to end
    patchJump(elseJump);                        // ->tiny jump
    emitByte(OP_POP);                           // cleanup the evaluated left side expr from stack
    parsePrecedence(PREC_OR);                   // parse the right side expression after OR
    patchJump(endJump);                         // ->jump to end
}

// parsing function for strings
// - the start+1 and end-2 and length-2 trim the leading: '"' and closing: '"'
// - then wrap that string in an Object, wrap that in a Value then push that to the constant-table.
static void string(bool _canAssign) {
    // we support escaping with '\' so we handle that here:
    int origLength = parser.previous.length - 2;    // -2 because we ignore opening and closing quotes:'"'
    int escapedLength = 0;
    char* escapedStr = ALLOCATE(char, origLength);  // will only shrink
    // loop all chars and combine '\'+'n' -> '\n' char, adjust total length:
    for (int i = 1; i < origLength + 1; ++i) {
        char c = parser.previous.start[i];
        // Loop every char and combine
        if (i < origLength && c == '\\') {
            char nextChar = parser.previous.start[++i];
            switch (nextChar) {
                case '\n':
                    break;
                case '\\':
                    escapedStr[escapedLength++] = '\\'; break;
                case '\'':
                    escapedStr[escapedLength++] = '\''; break;
                case '\"':
                    escapedStr[escapedLength++] = '\"'; break;
                case 'n':
                    escapedStr[escapedLength++] = '\n'; break;
                case 't':
                    escapedStr[escapedLength++] = '\t'; break;
                default:
                    break;
            }
            continue;
        }
        escapedStr[escapedLength++] = c;
    }
    emitConstant(OBJ_VAL(copyString(escapedStr, escapedLength)));
    FREE(char, escapedStr);  // we manually free our string we used to create the escaped string
}

// helper function for variable()
static void namedVariable(Token name, bool canAssign) {
    // Depending if were dealing with a local (arg !=-1) or a global we set OP_COMMANDS accordingly
    uint8_t getOp, setOp;
    int arg = resolveLocal(current, &name);
    if (arg != -1) {
        getOp = OP_GET_LOCAL;               // "... x + 99;""
        setOp = OP_SET_LOCAL;               // "x = 123;";
    } else if((arg = resolveUpvalue(current, &name)) != -1) {   // check outer-'local'-scopes
        getOp = OP_GET_UPVALUE;
        setOp = OP_SET_UPVALUE;
    } else {
        arg = identifierConstant(&name);    // get the idx to the value in globals-table
        getOp = OP_GET_GLOBAL;              // this will get global from table and push() it
        setOp = OP_SET_GLOBAL;              // this will set/assign the value from the expr to the existing global in the global-table
    }
    // then we do either the assignment (if '=' following), or just get the current value
    if (canAssign && match(TOKEN_EQUAL)) {
        expression();
        emitBytes(setOp, (uint8_t)arg);     
    } else {
        emitBytes(getOp, (uint8_t)arg);     
    }
}

// helper for classDeclaration() - create a token manually
static Token syntheticToken(const char* text) {
    Token token;
    token.start = text;
    token.length = (int)strlen(text);
    return token;
}

// parsing function for super-method calls - ex. "var method = super.someInheritedMethod;"
static void super_(bool canAssign) {
    if (currentClass == NULL) {
        error("Can't use 'super' outside of a class.");
    } else if(!currentClass->hasSuperclass) {
        error("Can't use 'super' in a class with no superclass.");
    }

    consume(TOKEN_DOT, "Expect '.' after 'super'.");
    consume(TOKEN_IDENTIFIER, "Expect superclass method name.");
    uint8_t name = identifierConstant(&parser.previous);    // last token was the identifier so we read that out

    namedVariable(syntheticToken("this"), false);
    //
    if (match(TOKEN_LEFT_PAREN)) {
        // to speed up method calls (That use super) we introduce a custom OP_SUPER_INVOKE
        uint8_t argCount = argumentList();
        namedVariable(syntheticToken("super"), false);
        emitBytes(OP_SUPER_INVOKE, name);
        emitByte(argCount);
    } else {
        // and fail back to the slow away (that can reslove var = fncall()) else
        namedVariable(syntheticToken("super"), false);
        emitBytes(OP_GET_SUPER, name);      // OP_GET_SUPER expects superclass on top of stack and below the receiver.
    }
}

// parsing function for resolving variables to their current value at runtime
// - the only time we allow an assignment is when parsing an assignment expression or top-level expression.
//      the canAssign FLAG makes it's way down to this lowest precedence expression (where we need it)
static void variable(bool canAssign) {
    namedVariable(parser.previous, canAssign);
}

// parsing function for this keyword - used in bound-methods to access class-fields variables.
// - we treat this as a lexically scoped local variable. (that needs not initialization)
static void this_(bool canAssign) {
    if (currentClass == NULL) {
        error("Can't use 'this' outside of a class.");   // cant use this at top level
        return;
    }
    variable(false);    // assigning to this is not possible (this = 12) so we pass canAssign=false
    // variable() treats "this" as if it were the variable identifier
}

// parsing function for an unary negation (-10 or !true)
static void unary(bool _canAssign) {
    TokenType operatorType = parser.previous.type;      // we need to differentiate between ! and -
    parsePrecedence(PREC_UNARY);                        // compiles the operand (ex: 10)

    // Emit the operator instruction (depending on ! or -)
    switch (operatorType) {
        case TOKEN_BANG:        emitByte(OP_NOT); break;
        case TOKEN_MINUS:       emitByte(OP_NEGATE); break;
        default: return;                                // Unreachable
    }
}

/* CUSTOM implementation on top of lox */

// parsing function for array initializations
// - we just parse everything separated by ',' push those values on stack
// - then we push the OpCode to build the array then the count 
static void arrayInit(bool canAssign) {
    int itemCount = 0;
    if (!check(TOKEN_RIGHT_BRACKET)) {
        do {
            if (check(TOKEN_RIGHT_BRACKET)){
                break;  // hit a trailing comma
            }
            parsePrecedence(PREC_OR);   // parses things between ','s and push values on stack
            if (itemCount == UINT8_COUNT) {
                error("Can't start Array with more than 256 entries.");
            }
            itemCount ++;
        } while (match(TOKEN_COMMA));
    }
    consume (TOKEN_RIGHT_BRACKET, "Expect ']' after array initialisation");
    emitByte(OP_ARRAY_BUILD);
    emitByte(itemCount);
}

// parsing function for array insertArr[idx] or assigning assignArr[idx] = true;
static void arrayEdit(bool canAssign) {
    parsePrecedence(PREC_OR);   // opening '[' already consumed so we expect the index next.
    consume(TOKEN_RIGHT_BRACKET, "Expect ']' after index.");
    if (canAssign & match(TOKEN_EQUAL)) {
        expression();                   // need to push the value that we gonna insert on the stack
        emitByte(OP_LISTS_WRITE_IDX);       // were writing into the array ex: 'someArr[10] = true'
    } else {
        emitByte(OP_LISTS_READ_IDX);    // were reading the value ex: 'someArr[10]'
    }
}


// parsing function for Map initialisation:
static void mapInit(bool canAssign) {
    int pairsCount = 0; // we count 1key and 1 value pair as 1.
    if (!check(TOKEN_RIGHT_BRACE)) {
        do {
            if(check(TOKEN_RIGHT_BRACE)) {
                break; // we hit a trailing comma
            }
            // key
            if (! check(TOKEN_STRING)) {
                error("Expect key string next inside Map.");
            }
            consume(TOKEN_STRING, "Expect key string inside Map");
            printf("\nstarting map init:\n");
            string(true);      //TODO: check but canAssign=false seems right

            consume(TOKEN_COLON, "Expect ':' between key and value of a Map.");
            // value:
            parsePrecedence(PREC_OR);
            if (pairsCount == UINT8_COUNT) {
                error("Can't start Map with more than 256 entries.");
            }
            pairsCount ++;
        } while(match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_BRACE, "Expect '}' after map initialisaton");
    emitByte(OP_MAP_BUILD);
    emitByte(pairsCount);
}


/*
*
*      Lookup table for Precedence (what operation gets executed first, what 2nd etc.)
*
*/

// This is just a lookuptable pointing Tokens to the functions that parse it.
// - prefix expressions are on the left side (get evaluated first) then poped on the stack
// - infix expressions are on the right side (get evaluated 2nd) then poped on the stack
// [TOKEN Name]      = {prefix-Fn, infix-Fn, Precedence Number }
ParseRule rules[] = {
    [TOKEN_LEFT_PAREN]    = {grouping,    call,      PREC_CALL},
    [TOKEN_RIGHT_PAREN]   = {NULL,        NULL,      PREC_NONE},
    [TOKEN_LEFT_BRACE]    = {mapInit,     NULL,      PREC_NONE},    // ={ will initialize Maps
    [TOKEN_RIGHT_BRACE]   = {NULL,        NULL,      PREC_NONE},
    [TOKEN_LEFT_BRACKET]  = {arrayInit,   arrayEdit, PREC_IDX_ARRAY},
    [TOKEN_RIGHT_BRACKET] = {NULL,        NULL,      PREC_NONE},
    [TOKEN_COMMA]         = {NULL,        NULL,      PREC_NONE},
    [TOKEN_DOT]           = {NULL,        dot,       PREC_CALL},
    [TOKEN_MINUS]         = {unary,       binary,    PREC_TERM},
    [TOKEN_PLUS]          = {NULL,        binary,    PREC_TERM},
    [TOKEN_SEMICOLON]     = {NULL,        NULL,      PREC_NONE},
    [TOKEN_SLASH]         = {NULL,        binary,    PREC_FACTOR},
    [TOKEN_STAR]          = {NULL,        binary,    PREC_FACTOR},
    [TOKEN_MODULO]        = {NULL,        binary,    PREC_FACTOR},
    [TOKEN_BANG]          = {unary,       NULL,      PREC_NONE},
    [TOKEN_BANG_EQUAL]    = {NULL,        binary,    PREC_EQUALITY},
    [TOKEN_EQUAL]         = {NULL,        NULL,      PREC_NONE},
    [TOKEN_EQUAL_EQUAL]   = {NULL,        binary,    PREC_EQUALITY},
    [TOKEN_GREATER]       = {NULL,        binary,    PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL,        binary,    PREC_COMPARISON},
    [TOKEN_LESS]          = {NULL,        binary,    PREC_COMPARISON},
    [TOKEN_LESS_EQUAL]    = {NULL,        binary,    PREC_COMPARISON},
    [TOKEN_IDENTIFIER]    = {variable,    NULL,      PREC_NONE},
    [TOKEN_STRING]        = {string,      NULL,      PREC_NONE},
    [TOKEN_NUMBER]        = {number,      NULL,      PREC_NONE},
    [TOKEN_AND]           = {NULL,        and_,      PREC_AND},
    [TOKEN_CLASS]         = {NULL,        NULL,      PREC_NONE},
    [TOKEN_ELSE]          = {NULL,        NULL,      PREC_NONE},
    [TOKEN_FALSE]         = {literal,     NULL,      PREC_NONE},
    [TOKEN_FOR]           = {NULL,        NULL,      PREC_NONE},
    [TOKEN_FUN]           = {NULL,        NULL,      PREC_NONE},
    [TOKEN_IF]            = {NULL,        NULL,      PREC_NONE},
    [TOKEN_NIL]           = {literal,     NULL,      PREC_NONE},
    [TOKEN_OR]            = {NULL,        or_,       PREC_OR},
    [TOKEN_PRINT]         = {NULL,        NULL,      PREC_NONE},
    [TOKEN_RETURN]        = {NULL,        NULL,      PREC_NONE},
    [TOKEN_SUPER]         = {super_,      NULL,      PREC_NONE},
    [TOKEN_THIS]          = {this_,       NULL,      PREC_NONE},
    [TOKEN_TRUE]          = {literal,     NULL,      PREC_NONE},
    [TOKEN_VAR]           = {NULL,        NULL,      PREC_NONE},
    [TOKEN_WHILE]         = {NULL,        NULL,      PREC_NONE},
    [TOKEN_ERROR]         = {NULL,        NULL,      PREC_NONE},
    [TOKEN_EOF]           = {NULL,        NULL,      PREC_NONE},
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
    bool canAssign = precedence <= PREC_ASSIGNMENT;             // need to pass this flag down to variable()
    prefixRule(canAssign);   // otherwise we call the Prefix-ParseFunction
    // that call will compile the rest of the prefix expression consuming any other tokens it needs

    // if the next token is too low precedence or isnt an infix operator were done.
    // otherwise  we consume the operand and off control to the infix parser we found ( to get the right side compiled)
    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
    }

    // we failed to match the left side of the `=` as valid assignment target:
    if (canAssign && match(TOKEN_EQUAL)) {
        error("Invalid assignment target.");        // ex: x*y=22;
    }
}


// this simply reads the corresponding ParseRule from our rules-lookuptable.
static ParseRule* getRule(TokenType type) {
    return &rules[type];
}

/*
*
*      The different types of parsing - expressions/statements:
*
*/

// helper for compile() 
// - Expressions always evaluate to a Value like 1+2->3 || True == "james"->False 
// - Implemented are so far: 
//      []Number-literals, []parentheses, []unary nengation, []Arithmetics: + - * /
static void expression() {
    parsePrecedence(PREC_ASSIGNMENT);
}

// helper for statement() - inner block enclosed by 2 curly-brackets   "{" block "}"
static void block() {
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        declaration();
    }
    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");      // Brackets must be closed again.
}

// helper for statement() - compiles a function "fun x() {print"hello"}"
static void function(FunctionType type) {
    Compiler compiler;
    initCompiler(&compiler, type);                              // we set a new Compiler that compiles (only) this function
    beginScope();

    consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
    if (!check(TOKEN_RIGHT_PAREN)) {
        // parameters is simply a local variable declared in the outermost lexical scope of a function body
        do {
            current->function->arity++; // we keep track of nr of parameters since we only max of 256
            if (current->function->arity > 255) {
                errorAtCurrent("Can't have more than 255 parameters.");
            }
            uint8_t constant = parseVariable("Expect parameter name.");
            defineVariable(constant);
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
    consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
    block();        // the function body, in a {}-block

    ObjFunction* function = endCompiler();                      // the Compiler for this function has finished -> we get the Function-Object from it
    emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(function)));     // the closure wraps our function
    // we emit instructions to resolve the Closure-captured variables to the actual point in memory where the underlying data is stored.
    for (int i=0; i<function->upvalueCount; i++) {
        emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
        emitByte(compiler.upvalues[i].index);
    }
}

// helper for classDeclaration() - parses a method inside a class body:
static void method() {
    consume(TOKEN_IDENTIFIER, "Expect method name.");
    uint8_t constant = identifierConstant(&parser.previous);
    // OP_METHOD needs: a objClosure (that function() pushes on the stack)
    // it will connect that function as a method to the class aboce it on the stack
    FunctionType type = TYPE_METHOD;
    if (parser.previous.length == 4 && memcmp(parser.previous.start, "init", 4)==0) {
        type = TYPE_INITIALIZER;    // most functions return implicit nil. BUT init() MUST return ALWAYS an INSTANCE!
    }
    function(type);
    emitBytes(OP_METHOD, constant);
}

// helper for declaration() - parses a class declaration: ex: "class Boats {var passengers = 10;}"
static void classDeclaration() {
    consume(TOKEN_IDENTIFIER, "Expect class name!");
    Token className = parser.previous;
    uint8_t nameConstant = identifierConstant(&parser.previous);
    declareVariable();                  // add our name ex. "Boats" to our string-lookup-table

    emitBytes(OP_CLASS, nameConstant);  // instruction to create Class Object at runtime
    defineVariable(nameConstant);       // OP_CLASS takes index of nametable to class-name

    ClassCompiler classCompiler;            // When the compiler begins to compile a class it pushes a new
    classCompiler.enclosing = currentClass; // classCompiler to that implicit linked stack (head is global)
    classCompiler.hasSuperclass = false;    // we assume we do not inherit (as default).
    currentClass = &classCompiler;

    // next we check for inheritance: (syntax is: "class Cat < Pet {...}")
    if (match(TOKEN_LESS)) {
        consume(TOKEN_IDENTIFIER, "Expect superclass name.");   // first we consume the identifier '(ex Pet)
        variable(false);                                        // takes previous token as variable reference-> pushes superclass on stack
        if (identifiersEqual(&className, &parser.previous)) {
            error("A class can't inherit from itself.");
        }
        beginScope();                                       // handle Superclass calls. first we create a local scope. (needed when 2 classes share the same scope)            
        addLocal(syntheticToken("super"));                  // then we 'reserve' a local variable calls "super" by pushing that on the stack
        defineVariable(0);                                  // and make it a local variable
        namedVariable(className, false);                        // then we emit our opcode for inheritance
        emitByte(OP_INHERIT);
        classCompiler.hasSuperclass = true;                 // we set our bool to signal we inherited -> made local scope (-> we need to cleanup that later)
    }
    namedVariable(className, false);    // method needs the class identifier-name above it on the stack:
    consume(TOKEN_LEFT_BRACE, "Expect '{' before class body!");
    // we check for method-declaration/initialisation, ex: getname():  "class Bob { getName() { return "Bob";}}""
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        method();
    }
    consume(TOKEN_RIGHT_BRACE, "Expect '}' after class body!");
    emitByte(OP_POP);                   // we only pushed the class identifer-name for method() so we pop it after
    if (classCompiler.hasSuperclass) {
        endScope();                                         // we have to remove the scope we created when handling superclasses
    }
    currentClass = currentClass->enclosing; // when were done we remove that from the implicit linked stack
}

// helper for declaration() - parses a Function declaration: ex: "fun doStuff() {...}"
// - a function declaration at top lvl will bind the function to a global variable
// - a function inside a block or other function creates a local variable
static void funDeclaration() {
    uint8_t global = parseVariable("Expect function name.");
    markInitialized();          // we can instantly mark the function initialized -> this enables recursion.
    function(TYPE_FUNCTION);
    defineVariable(global);
}

// helper for declaration() - initial declaration of variables
static void varDeclaration() {
    uint8_t global = parseVariable("Expect variable name.");

    if (match(TOKEN_EQUAL)) {
        expression();       // pops initial value on the stack
    } else {
        emitByte(OP_NIL);   // uninit -> we manually pop NIL on the stack ('var a' becomes 'var a=nil')
    }
    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
    defineVariable(global);
}

// "eat("peaches");" is simply an expression followed by a semicolon. 
// - Usually to call it's side effects(ex function-call); 
static void expressionStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OP_POP);   // discards the result from the stack. Since we are only after side effects.
}

// parse for loops -    "for (var i=0; i<10; i=x+1) print x;"   but also    "for (;;) {doInfiniteLoop;}""
static void forStatement() {
    beginScope();                   // we ensure the counter variable is a local variable
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
    // Initializer clause           // "for(x=0;...;..)":
    if (match(TOKEN_SEMICOLON)) {   // check if this (optinal clause) exists
        // No initializer found     // ex.: for (;x>10;x++)
    } else if (match(TOKEN_VAR)) {
        varDeclaration();           // ex.: for(var x=0;x>10;x++)
    } else {
        expressionStatement();      // ex.: for(x=0;x>10;x++)
    }
    int loopStart = currentChunk()->count;
    // Condition clause             // "for(..;x>10;..)":
    int exitJump = -1;
    if (!match(TOKEN_SEMICOLON)) {  // check if this (optinal clause) exists
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");
        // jump out of the loop if exit condition is true:
        exitJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP);           // clear the exit-condition from stack if were not jumping
    }
    // Increment clause             // "for(..;...;x=x+10)":
    // - we will jump over the increment, run the body, 
    // - then jump back to the increment run it then go to the next iteration.
    if (!match(TOKEN_RIGHT_PAREN)) {// check if this (optional clause) exists
        int bodyJump = emitJump(OP_JUMP);
        int incrementStart = currentChunk()->count;
        expression();
        emitByte(OP_POP);
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after for increment clause.");

        emitLoop(loopStart);
        loopStart = incrementStart;
        patchJump(bodyJump);
    }

    statement();                    // the 'body' of the loop that gets repeated
    emitLoop(loopStart);
    if (exitJump != -1) {
        patchJump(exitJump);
        emitByte(OP_POP);           // clear the exit-condition from stack. (after we jumped to end)
    }
    endScope();                     // we needed a local scope for our loop (counter variable)
}

// parses ifStatements - "if (isTrue) { // then do this; }"
static void ifStatement() {
    // the condition, enclosed by round-brackets  "(expression)"
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");
    // we emit the JUMP IF FALSE -> so we skipp our statement if the previous expression evals to false:
    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);                       //cleanup the condition value on the stack (expr==false case)
    statement();
    int elseJump = emitJump(OP_JUMP);
    patchJump(thenJump);                    // this will skip the statement()-bytecode-instructions if expr==false
    emitByte(OP_POP);                       // cleanup the condition value on the stack (expr==true/ELSE case)
    if (match(TOKEN_ELSE)) statement();     // IF...ELSE... Should ONLY execute when expr==true
    patchJump(elseJump);                    // so we skipp the above bytecode instructions IF expr==true
}

// "print x + 1;" -> will eval x+1 then print that;
static void printStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(OP_PRINT);
}

// "return true;" -> will exit this function scope to one level lower.
static void returnStatement() {
    if (current->type == TYPE_SCRIPT) {
        error("Can't return from top-level code.");
    }
    if (match(TOKEN_SEMICOLON)) {
        emitReturn();                       // return; (-> implicit NIL from emitReturn())
    } else {
        if(current->type == TYPE_INITIALIZER) {
            error("Can't return a value from an initializer.");
        }
        expression();                       // return expr;
        consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
        emitByte(OP_RETURN);
    }
}

// 'while (true) print"loop is running"; '
//  - we skipp over the statement with a Jump if the while condition is false
static void whileStatement() {
    int loopStart = currentChunk()->count;      // loop will jump back to this value
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");
    int exitJump = emitJump(OP_JUMP_IF_FALSE);  // our jump skip/leave the whole loop
    emitByte(OP_POP);
    statement();
    emitLoop(loopStart);
    patchJump(exitJump);                        // we exit with this once while expr==false
    emitByte(OP_POP);
}

// helper for declaration() - after error we enter panic mode and try to get back to a valid state
// - we just eat tokens till we hit a statement boundary. then exit panic mode.
static void synchronize() {
    parser.panicMode = false;
    while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON) return;
        switch (parser.current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;
            default:
                ;   // DO NOTHING
        }
        advance();
    }
}

// maps different declaration (from our parsing grammar)
//  declaration     -> classDecl | funDecl | varDecl | statement;
static void declaration() {
    if(match(TOKEN_CLASS)) {
        classDeclaration();
    } else if (match(TOKEN_FUN)) {
        funDeclaration();
    } else if (match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        statement();                            // tries to parse tokens to find a statement
    }
    
    if (parser.panicMode) synchronize();        // encountered error -> we try to get back to a good state.
}

// Maps different kinds of statements (from our parsing grammar)
// statement        ->exprStmt | forStmt | ifStmt | printStmt | returnStmt | whileStmt | block;
// block            -> "{" declaration "}"
static void statement() {
    if (match(TOKEN_PRINT)) {
        printStatement();
    } else if (match(TOKEN_FOR)) {
        forStatement();
    } else if (match(TOKEN_IF)) {
        ifStatement();
    } else if (match(TOKEN_RETURN)) {
        returnStatement();
    } else if (match(TOKEN_WHILE)) {
        whileStatement();
    } else if (match(TOKEN_LEFT_BRACE)) {       // we need to go one scope deeper (block encountered)
        beginScope();
        block();
        endScope();
    } else {
        expressionStatement();
    }
}

/*
*
*       The 'Main' compile function
*
*/

// we pass in the source code string, then try to compile the source 
// - we compile bytecode and write it to the Chunk (that is stored in the ObjFunction, that is stored in the Compiler)
// - to enable functions (that may exists in toplevel or another function) we return the compiled ObjFunction* 
//      - we treat toplevel as a ObjFunction with name NULL
// - if compilation fails we return false (compilation error) to upstread disregard the whole chunk
ObjFunction* compile(const char* source) {
    initScanner(source);
    Compiler compiler;                          // set up our compiler
    initCompiler(&compiler, TYPE_SCRIPT);       // we start compiling top-level (TYPE_SCRIPT)
    // 'initialize' our Error-FLAGS:
    parser.hadError = false;
    parser.panicMode = false;

    advance();                                  // primes the scanner
    while (!match(TOKEN_EOF)) {
        declaration();                          // this will consume tokens and try to find declaration -> statements etc. (accoring to our lox-syntax)
    }
    ObjFunction* function = endCompiler();      // Calls and Functions call-end-compiler
    return parser.hadError ? NULL : function;   //  if we encountered compile-time-errors we return NULL, else return the ObjFunction with the bytecode
}

// used for GC - if GC starts while were still compiling -> we need to GC the compiler-structs aswell
// - so we walk all those and to mark every Obj still referenced
void markCompilerRoots() {
    Compiler* compiler = current;
    while (compiler != NULL) {
        markObject((Obj*)compiler->function);
        compiler = compiler->enclosing;
    }
}