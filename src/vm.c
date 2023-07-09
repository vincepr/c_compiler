#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "object.h"
#include "memory.h"
#include "vm.h"

// instance of our VM:
VM vm;

// define Static/Native C-Functions - returns time elapsed since the program started running in seconds.
// - in lox its available with: 'clock()'
static Value clockNative(int argCount, Value* args) {
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

// helperFunction to setup/reset the stack
static void resetStack() {
    vm.stackTop = vm.stack;     // we just reuse the stack. So we can just point to its start
    vm.frameCount = 0;          // CallFrame stack is empty when the vm starts up)
    vm.openUpvalues = NULL;     // empty 'linked list'
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

    // Extract info from current CallStack and print out.
    // - then we walk the stack top->bottom and find the line number,
    //      that corresponds to the current ip and print that line number along with function name
    for (int i = vm.frameCount - 1; i>=0; i--) {
        CallFrame* frame = &vm.frames[i];
        ObjFunction* function = frame->closure->function;
        size_t instruction = frame->ip - function->chunk.code -1;
        fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);
        if (function->name == NULL) {
            fprintf(stderr, "script\n");
        } else {
            fprintf(stderr, "%s()\n", function->name->chars);
        }
    }
    resetStack();
}

// takes pointer to a C-Function and the name it will be known as in Lox. 
// We Wrap the function in an ObjNative then store that in a global Variable (that our code can call)
// -  we push and pop the name and function on the stack -> this is so the GC will not free anything in use
static void defineNative(const char* name, NativeFn function) {
    push(OBJ_VAL(copyString(name, (int)strlen(name))));
    push(OBJ_VAL(newNative(function)));
    tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
}

void initVM() {
    resetStack();
    vm.objects = NULL;      // reset linked list of all active object
    vm.bytesAllocated = 0;
    vm.nextGC = 1024 * 1024;// the first GC will get triggered when Heap gets bigger than this value
    vm.grayCount = 0;       // init the gray-Stack we use in our GC-Algorithm:
    vm.grayCapacity = 0;
    vm.grayStack = NULL;
    initTable(&vm.globals); // setup the HashTable for global variables
    initTable(&vm.strings); // setup the HashTable for used strings

    // init Native Functions:
    defineNative("clock", clockNative);
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

// initializes the next CallFrame on the stack
// - stores pointer to the function beeing called and points the frame's ip to the beginning of that functions bytecode
// - then it sets up slots pointer to give the frame it's window on the stack.
static bool call(ObjClosure* closure, int argCount) {
    // ErrorChecking "fun do(a,b,c){} do(1,2)" -> called with wrong nr Parameters
    if (argCount != closure->function->arity) {
        runtimeError("Expected %d arguments but got %d.", closure->function->arity, argCount);
        return false;
    }
    if (vm.frameCount == FRAMES_MAX) {
        runtimeError("Stack overflow.");
        return false;
    }
    // Setup the Stack-Frame:
    CallFrame* frame = &vm.frames[vm.frameCount++];         //  prepare an initial CallFrame 
    frame->closure = closure;                               //  in the new CallFrame we point to the function
    frame->ip = closure->function->chunk.code;              //  initialize its ip to the beginning of the functions bytecode
    //                                                          and set up its stack window to start at the bottom of the VM's value stack 
    frame->slots = vm.stackTop - argCount -1;               // -1 because of the reserved 0-idx stack slot(reserved for methods-calls)
                                                                
    return true;
}

// calls a Function or Class - maps all supported objects that can call or return false on error
static bool callValue(Value callee, int argCount) {
    if (IS_OBJ(callee)) {
        switch(OBJ_TYPE(callee)) {
        // since lox is a dynamic language, we need to check types calling a Function/Method at runtime-type. 
        // - to block: "a_string"(); or "var x = 123; x()" 
            case OBJ_CLASS: {
                // To instance a Class we reuse callValue - instead of 'new SomeClass' we do 'SomeClass()'
                ObjClass* pClass = AS_CLASS(callee);
                vm.stackTop[-argCount -1] = OBJ_VAL(newInstance(pClass));   
                return true;
            }
            case OBJ_CLOSURE:
                return call(AS_CLOSURE(callee), argCount);
            case OBJ_NATIVE: {
                // if the object being called is a native function -> invoke the C-Function right there
                NativeFn native = AS_NATIVE(callee);
                Value result = native(argCount, vm.stackTop - argCount);
                vm.stackTop -= argCount + 1;
                push(result);   // we use the result from the C-Function and stuff it back in the stack
                return true;
            }
            default:
                break;  // Non-callable object type tried to call ex.: "just string"()
        }
    }
    runtimeError("Can only call functions and classes.");
    return false;
}

// creates a new Upvalue for captured local variable:
static ObjUpvalue* captureUpvalue(Value* local) {
    // walk the linked-list of active/openUpvalues - to add it if new
    ObjUpvalue* prevUpvalue = NULL;
    ObjUpvalue* upvalue = vm.openUpvalues;
    // we check for an upvalue with slot below the one were looking for -> gone past where were closing over-> not found
    while (upvalue != NULL && upvalue->location > local) {
        prevUpvalue = upvalue;      // add it if new to list
        upvalue = upvalue->next;
    }
    if (upvalue != NULL && upvalue->location == local)  {
        return upvalue;             // or return reference if local is already captured
    }
    ObjUpvalue* createdUpvalue = newUpvalue(local);  // allocates the new upvalue
    // otherwise we create a new upvalue for our local slot and insert it into the list at the right locaton:
    createdUpvalue->next = upvalue;
    if (prevUpvalue == NULL) {
        vm.openUpvalues = createdUpvalue;   // we hit end of list -> we insert at head of list
    } else {
        prevUpvalue->next = createdUpvalue; // we insert between the previous node and next
    }
    return createdUpvalue;
}

// when a function returns local-vars will close -> need to be captured if used in Upvalue
// - closes every open upvalue it can find, that points to that slot or above(on the stack)
// - we walk list of open Upvalues till we hit range while closing any we find
static void closeUpvalues(Value* last) {
    while (vm.openUpvalues != NULL && vm.openUpvalues->location >= last) {
        ObjUpvalue* upvalue = vm.openUpvalues;
        // 
        upvalue->closed = *upvalue->location;       // close the upvalue by: copying value to closed field
        upvalue->location = &upvalue->closed;       // instead of pointing to where stack-variable we now point to itself->closed 
        vm.openUpvalues = upvalue->next;
    }
}

// Connects a method (closure on stack) to its class at runtime
// - the method closure is on top of the stack, below it the class we bind it to
static void defineMethod(ObjString* name) {
    Value method = peek(0);                         // read method from stack
    ObjClass* pClass = AS_CLASS(peek(1));           // read its parent Class
    tableSet(&pClass->methods, name, method);       // write the closure in the method table of class
    pop();                                          // we dont need closure name on the stack anymore
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
    ObjString* b = AS_STRING(peek(0));  // we read and temporarily it but leave it on the stack
    ObjString* a = AS_STRING(peek(1));  // to make sure GC can find it

    int length = a->length + b->length;
    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString* result = takeString(chars, length);
    pop();                              // we pop the 2 string objects from the stack
    pop();                              // that we only left there for GC safety
    push(OBJ_VAL(result));
}


// helper function for interpret() that actually runs the current instruction
static InterpretResult run() {
    // instance of our CallFrame:
    CallFrame* frame = &vm.frames[vm.frameCount - 1];
// macro-READ_BYTE reads the byte currently pointed at by the instruction-pointer(ip) then advances the ip.
#define READ_BYTE() (*frame->ip++)
// reads last 2 8-bit-chunks and interprets it as a 16-bit int.
#define READ_SHORT() \
    (frame->ip += 2, \
    (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
// macro-READ_CONSTANT: reads the next byte from the bytecoat, treats it number as index and looks it up in our constant-pool
#define READ_CONSTANT() (frame->closure->function->chunk.constants.values[READ_BYTE()])
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
            if (FLAG_TRACE_EXECUTION) {
                // loop over stack and show its contents:
                printf("          ");
                for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                    printf("[ ");
                    printValue(*slot);
                    printf(" ]");
                }
                printf("\n");
                // show the disassembled/interpreted instruction
                disassembleInstruction(&frame->closure->function->chunk, (int)(frame->ip - frame->closure->function->chunk.code));
            }
        #endif

        // first byte of each instruction is opcode so we decode/dispatch it:
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
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
            case OP_GET_LOCAL: {                    // read current local value and push it on the stack
                uint8_t slot = READ_BYTE();
                push(frame->slots[slot]);
                break;
            }
            case OP_SET_LOCAL: {                    // writes to local-variable the top value on the stack.(doesnt touch top of stack)
                uint8_t slot = READ_BYTE();
                frame->slots[slot] = peek(0);
                break;
            }
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
            case OP_SET_UPVALUE: {                  // we take the value on top of the stack and store it into the slot pointed by upvalue
                uint8_t slot = READ_BYTE();
                *frame->closure->upvalues[slot]->location = peek(0);
                break;
            } 
            case OP_GET_UPVALUE: {                  // resolves the underlying value from a Enclosed Upvalue (variable used by closure)
                uint8_t slot = READ_BYTE();
                push(*frame->closure->upvalues[slot]->location);
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
            case OP_JUMP: {                 // reads offset of the jump forward. then jumps without any checks.
                uint16_t offset = READ_SHORT();
                frame->ip += offset;
                break;
            }
            case OP_JUMP_IF_FALSE: {        // reads offset of the jump forward to it if statement on stack is falsey.
                uint16_t offset = READ_SHORT();
                if (isFalsey(peek(0))) frame->ip += offset;
                break;
            }
            case OP_LOOP: {                 // unconditionally jumps back to the 16-bit offset that follows in 2 8bit chunks afterwards
                uint16_t offset = READ_SHORT();
                frame->ip -= offset;
                break;
            }
            case OP_CALL: {                 // reads nr of parameters/arguments from stack -> this is the start of the function on the stack
                int argCount = READ_BYTE();
                if (!callValue(peek(argCount), argCount)) {
                    return INTERPRET_RUNTIME_ERROR;     // if callValue() -> false we know a runtime error happened
                }
                frame = &vm.frames[vm.frameCount - 1];  // there will be a new frame on the CallFrame stack for the called function, that we update
                break;
            }
            case OP_CLOSURE: {                           
                ObjFunction* function = AS_FUNCTION(READ_CONSTANT());   // load the compiled function from the const-table
                ObjClosure* closure = newClosure(function);             // -> wrap it in ObjClosure
                push(OBJ_VAL(closure));                                 // -> and push it to the stack
                // we iterate over each upvalue the closure expects:
                for (int i=0; i<closure->upvalueCount; i++) {
                    // Read the pair of 2 Bytes of data after the OP_CLOSURE from the stack (isLocal/isNotAnotherUpvalue and index)
                    uint8_t isLocal = READ_BYTE();
                    uint8_t index = READ_BYTE();
                    if (isLocal) {
                        closure->upvalues[i] = captureUpvalue(frame->slots + index); // if upvalue closes over a local variable
                    } else {
                        closure->upvalues[i] = frame->closure->upvalues[index]; // otherwise we capture an from surrounding function
                    }
                }
                break;
            }
            case OP_CLOSE_UPVALUE:              // we have to hoist a local-variable to the heap (because of closure)
                closeUpvalues(vm.stackTop -1);
                pop();
                break;
            case OP_GET_PROPERTY: {             // expression to the left of dot has already executed (instance on stack)
                // we have to check against non-instances calling this: 'var x = true; print x.fakeField;'
                if (!IS_INSTANCE(peek(0))) {
                    runtimeError("Only instances have properties.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjInstance* instance = AS_INSTANCE(peek(0));
                ObjString* name = READ_STRING();
                Value value;
                //                              we read the field name from name-lookuptable:
                if (tableGet(&instance->fields, name, &value)) {
                    pop();                      // if variable exists we pop it
                    push(value);                // and push the value of the variable
                    break;
                }
                //                              if field doesnt exists we runtime error:
                runtimeError("Undefined property '%s'.", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_SET_PROPERTY: {
                // we have to check against non-instances calling this: 'var x=false; var x.y = true;'
                if (!IS_INSTANCE(peek(1))) {
                    runtimeError("Only instances have fields.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                // when called Stack top looks like this -> instance | value to be stored | ... 
                ObjInstance* instance = AS_INSTANCE(peek(1));       // get the field name
                // store the value on top of the stack into instance field table
                tableSet(&instance->fields, READ_STRING(), peek(0));
                Value value = pop();
                pop();
                push(value);    // here basically leave top element on stack but remove the one one below that
                break;          // this is done because a setter is itself an expression
            }
            
            // OP_RETURN - when a function returns a value that value will be currently on the top of the stack
            // - so we can pop that value, then dispose of the whole functions StackFrame
            case OP_RETURN: {
                Value result = pop();
                closeUpvalues(frame->slots);    // when a function returns local-vars will close -> need to be captured if used in Upvalue
                vm.frameCount--;
                if(vm.frameCount == 0) {
                    // if we reached the last CallFrame it means we finished executing the top-level code -> the program is done.
                    pop();          // so we pop the main script from the stack and exit the interpreter
                    return INTERPRET_OK;
                } 
                vm.stackTop = frame->slots;
                push(result);   // we push that result of the finished function back on the stack. (one level lower)
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }   
            // creates Runtime class-object is followed by idx for name-table to class-name-identifier
            // - it just loads string for that class and pass that to newClass()
            case OP_CLASS:
                push(OBJ_VAL(newClass(READ_STRING())));
                break;
            case OP_METHOD:
                defineMethod(READ_STRING());
                break;
        }
    }
// we only need our macros in run() so we scope them explicity to only be available here:
#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}

// takes the source-code string (from file or repl) and interprets/runs it
InterpretResult interpret(const char* source) {
    ObjFunction* function = compile(source);                // pass SourceCode to compiler
    if (function == NULL) return INTERPRET_COMPILE_ERROR;   // NULL means we hit some kind of Compile-Error(that Compiler already reported)

    push(OBJ_VAL(function));                                // store the funcion on the stack
    ObjClosure* closure = newClosure(function);             // wrap the function in its wrapper Closure (vm, only touches closures not functions)
    pop();                                                  // pop the created Function
    push(OBJ_VAL(closure));                                 // and push the closure (so its there instead the function) this happens for gc-reasons
    call(closure, 0);                                       // initializes the toplevel Stack-Frame

    return run();
}