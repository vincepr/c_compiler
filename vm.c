#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "object.h"
#include "memory.h"
#include "vm.h"

// instance of our VM:
VM vm;

// helperFunction to setup/reset the stack
static void resetStack() {
    vm.stackTop = vm.stack;     // we just reuse the stack. So we can just point to its start
}

// error Handling of Runtime Errors (like trying to - negate a bool)
// - takes varrying number of args
// - callers can pass a format string followed by a number of args. These args then get printed out.
static void runtimeError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args); 
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = vm.ip - vm.chunk->code -1; // interpreter +1's before interpreting. so we have to -1 to corretly locate the error
    int line = vm.chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);
    resetStack();
}

void initVM() {
    resetStack();
    vm.objects = NULL;      // reset linked list of all active objects
    initTable(&vm.globals); // setup the HashTable for global variables
    initTable(&vm.strings); // setup the HashTable for used strings
}

void freeVM() {
    freeTable(&vm.globals);
    freeTable(&vm.strings);
    freeObjects();          // when free the vm, we need to free all objects in the linked-list of objects.
}

// push a value to our Value-Stack
void push(Value value) {
    *vm.stackTop = value;   // add element to the top of our stack
    vm.stackTop++;          // increment the pointer to point to the next element (above just added one)
}

// pop a value from our Value-Stack
Value pop() {
    vm.stackTop--;          // since our stackTop points to 'above' the last element
    return *vm.stackTop;    // we can decrement first then retreive the top/removed element
}

// returns Value from the stack WITHOUT poping it. (distance is an offset from the top) (0 is the top-one)
static Value peek(int distance) {
    return vm.stackTop[-1-distance];
}

// we handle what Values may be evaluated as a boolean.
// - nil and false are falsey.
// - every other value behaves like true.
static bool isFalsey(Value value) {
    return IS_NIL(value) || ( IS_BOOL(value) && !AS_BOOL(value) );
}

// Concatenate two strings
// - calculate length of result string
// - allocate char-array for the result (with calculated length)
// - copy into our result first a, then b, then the Nullterminator:'\0'
static void concatenate() {
    ObjString* b = AS_STRING(pop());
    ObjString* a = AS_STRING(pop());

    int length = a->length + b->length;
    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString* result = takeString(chars, length);
    push(OBJ_VAL(result));
}


// helper function for interpret() that actually runs the current instruciton
static InterpretResult run() {
// macro-READ_BYTE reads the byte currently pointed at by the instruction-pointer(ip) then advances the ip.
#define READ_BYTE() (*vm.ip++)
// macro-READ_CONSTANT: reads the next byte from the bytecoat, treats it number as index and looks it up in our constant-pool
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
// macro reads one-byte from the chunk, reats it as idex into the constants-table -> gets that string
#define READ_STRING() AS_STRING(READ_CONSTANT())
// macro-Enables all Arithmetic Functions (since only difference is the sign +-/* for the most part) - is this preprocessor abuse?!?
// - first we check that the two operands(left and right) are numbers. ->if yes we Error out.
// - if not, we pop the 2 structs unwrap them (struct->C-double) 
// - then do the calculation and push that on the stack
#define BINARY_OP(valueType, op) \
    do{ \
        if ( !IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1)) ) { \
            runtimeError("Operands must be numbers."); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
        double b = AS_NUMBER(pop()); \
        double a = AS_NUMBER(pop()); \
        push(valueType(a op b)); \
    } while (false);

    for(;;) {
        // support for the Debug-Flag to enable printing out diagnostics:
        #ifdef DEBUG_TRACE_EXECUTION
            // loop over stack and show its contents:
            printf("          ");
            for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                printf("[ ");
                printValue(*slot);
                printf(" ]");
            }
            printf("\n");
            // show the disassembled/interpreted instruction
            disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
        #endif

        // first byte of each instruction is opcode so we decode/dispatch it:
        uint8_t instruciton;
        switch (instruciton = READ_BYTE()) {
            // OP_CONSTANT - fixed values,l ike x=3
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push(constant);             // push the constant/tempory-value to the stack
                break;
            }
            // true, false, nil just push the corresponding value on the stack:
            case OP_NIL:        push(NIL_VAL); break;
            case OP_TRUE:       push(BOOL_VAL(true)); break;
            case OP_FALSE:      push(BOOL_VAL(false)); break;
            // stack operations:
            case OP_POP:        pop(); break;       // pop value from stack and forget it.
            case OP_GET_GLOBAL: {                   // get value for named-variable and push it on stack.
                ObjString* name = READ_STRING();
                Value value;
                // if key isnt present, that means the variable has not been defined -> runtime error:
                if (!tableGet(&vm.globals, name, &value)) {
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value);
                break;
            }
            case OP_DEFINE_GLOBAL: {                // pop the last val from stack and write it to our globals table
                ObjString* name = READ_STRING();    // get var identifier from constant table
                tableSet(&vm.globals, name, peek(0));
                pop();
                break;
            }
            case OP_SET_GLOBAL: {                   // Try to write to existing global variable
                ObjString* name = READ_STRING();
                if (tableSet(&vm.globals, name, peek(0))) {
                    tableDelete(&vm.globals, name); // delete zombie values from table (important for REPL)
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                // no pop() from the stack, since the assignment could be nested in some larger expression
                break;
            }
            //  comparisons:
            case OP_EQUAL: {
                Value b = pop();
                Value a = pop();
                push(BOOL_VAL(valuesEqual(a, b)));
                break;
            }
            case OP_GREATER:    BINARY_OP(BOOL_VAL, >); break;
            case OP_LESS:       BINARY_OP(BOOL_VAL, <); break;
            // binary operations - arithmetic:
            case OP_ADD: {
                if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                    concatenate();      // string + x -> contatenate together
                } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    double b = AS_NUMBER(pop());
                    double a = AS_NUMBER(pop());
                    push(NUMBER_VAL(a + b));
                } else {
                    runtimeError("Operands must be two numbers or two strings.");
                }
                break;
            }
            case OP_SUBTRACT:   BINARY_OP(NUMBER_VAL, -); break;
            case OP_MULTIPLY:   BINARY_OP(NUMBER_VAL, *); break;
            case OP_DIVIDE:     BINARY_OP(NUMBER_VAL, /); break;
            // OP_NOT - logical negation: we just pop one operand, negate it then push result back.
            case OP_NOT:
                push(BOOL_VAL(isFalsey(pop())));
                break;
            // OP_NEGATE - arithmetic negation - unary expression, like -x with x=3 -> -3:
            case OP_NEGATE: 
                // TODO: no {} here! check if this is on purpose? does it make a difference in c?
                if (!IS_NUMBER(peek(0))) {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUMBER_VAL(-AS_NUMBER(pop())));
                break;
            // OP_PRINT - console.log() prints to terminal. Like 'print "hello";' -> "hello\n" to Terminal
            case OP_PRINT: {
                printValue(pop());
                printf("\n");
                break;
            }
            // OP_RETURN - exits the loop entirely (end of chunk reached/return from the current Lox function)
            case OP_RETURN: {
                return INTERPRET_OK;
            }
        }
    }

// we only need our macros in run() so we scope them explicity to only be available here:
#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}

// takes the source-code string (from file or repl) and interprets/runs it
InterpretResult interpret(const char* source) {
    Chunk chunk;
    initChunk(&chunk);

    // if compile functions fails (returns false) we discard the unusable chunk -> compile time error
    if (!compile(source, &chunk)) {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }
    // otherwise we send the completed chunk over to the vm to be executed:
    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;
    InterpretResult result = run();
    // and do some cleanup:
    freeChunk(&chunk);
    return result;
}