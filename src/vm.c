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
#include "array.h"

// instance of our VM:
VM vm;

// foward declaration:
static void runtimeError(const char* format, ...);


// define Static/Native C-Functions - returns time elapsed since the program started running in seconds.
// - in lox its available with: 'clock()'
static NativeResult clockNative(int argCount, Value* args) {
    NativeResult result;
    result.value = NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
    result.didError = false;
    return result;
}

/* CUSTOM Native functions added on top of default lox implementation: */ 

// runtime typechecking
static NativeResult typeofNative(int argCount, Value* args) {
    NativeResult result;
    result.value = NIL_VAL;
    result.didError = false;
    if (argCount != 1){
        runtimeError("typeof(value) expects exactly one argument.");
        result.didError = true;
        return result;
    }
    const char* ch;
    if (IS_NUMBER(args[0])){
        ch = "number";
    } else if (IS_BOOL(args[0])) {
        ch = "bool";
    } else if (IS_STRING(args[0])){
        ch = "string";
    } else if (IS_NIL(args[0])) {
        ch = "nil";
    } else if (IS_ARRAY(args[0])) {
        ch = "array";
    } else if (IS_INSTANCE(args[0])) {
        ObjInstance* inst = AS_INSTANCE(args[0]);
        result.value = OBJ_VAL( inst->pClass->name);    // if we hit an instance we return class-name for now
        return result;
    } else if (IS_CLOSURE(args[0])) {
        ch = "fun";
    } else {
        ch = "object";      // we just return obj for everything else
    }
    result.value = OBJ_VAL( copyString(ch, (int)strlen(ch)));
    return result;
}

// dirty printf() to print multiple different values in one function (no automatic newline)
// - a seperate OP-Code would be slightly faster/more efficient
static NativeResult printfNative(int argCount, Value* args) {
    NativeResult result;
    result.value = NIL_VAL;
    result.didError = false;
    if (argCount < 1) {
        runtimeError("wrong use of printf(...args) - Need at least 1 argument.");
        result.didError = true;
        return result;
    }

    for (int count=0; count < argCount; count++) {
        //printf("count: %d\n", count);
        if (IS_NUMBER(args[count]) || IS_STRING(args[count]) || IS_BOOL(args[count]) || IS_NIL(args[count]) 
            || IS_CLASS(args[count]) || IS_INSTANCE(args[count]) ||  IS_ARRAY(args[count]) || IS_OBJ(args[count])) {
            Value value = args[count];
            printValue(value);
        } else {
            runtimeError("wrong argument in printf(...args), can't print arg: %d.", count + 1);
            result.didError = true;
            return result;
        }
    }
    return result;
} 

// shitty version of rounding (but i really dont want to use any extern libraries)
// - at least it works for small numbers
static double myfloor(double num) {
    long long n = (long long)num;
    double d = (double)n;
    if (d == num || num >= 0)
        return d;
    else
        return d - 1;
}
static NativeResult floorNative(int argCount, Value* args) {
    NativeResult result;
    result.didError = false;
    if (argCount != 1 || !IS_NUMBER(args[0])) {
        runtimeError("'floor()' can only round numbers.");
        result.didError = true;
        result.value = NIL_VAL;
        return result;
    }
    double x = AS_NUMBER(args[0]);
    result.value = NUMBER_VAL(myfloor(x));
    return result;
}
// custom float Modulo ( does not work for negatives)
float myFloatModulo(float a, float b){
    return (a - b * myfloor(a / b));
}

// len(array/string) - getting length of array or string
static NativeResult lengthNative(int argCount, Value* args) {
    NativeResult result;
    result.didError = false;
    if (argCount == 1 && IS_ARRAY(args[0])) {
        ObjArray* array = AS_ARRAY(args[0]);
        int count = arrayGetLength(array);
        result.value = NUMBER_VAL((double)count);
        return result;
    } else if (argCount == 1 && IS_STRING(args[0])) {
        ObjString* string = AS_STRING(args[0]);
        result.value = NUMBER_VAL((double)string->length);
        return result;
    } else {

        runtimeError("'len()' can only get length from array or string.");
        result.didError = true;
        result.value = NIL_VAL;
        return result;
    }
}

// push(array, value) - ads push functionality to array, adds element on top
static NativeResult arrPushNative(int argCount, Value* args) {
    NativeResult result;
    result.didError = false;
    if (argCount != 2 || !IS_ARRAY(args[0])) {
        runtimeError("wrong arguments for: 'push(array, 123)'.");
        result.didError = true;
        result.value = NIL_VAL;
        return result;
    }
    ObjArray* array = AS_ARRAY(args[0]);
    Value item = args[1];
    arrayAppendAtEnd(array, item);
    result.value = NIL_VAL;
    return result;
}

// pop(array) - adds pop functionality to array, removes top and returns it
static NativeResult arrPopNative(int argCount, Value* args) {
    NativeResult result;
    result.didError = false;
    if (argCount != 1 || !IS_ARRAY(args[0])) {
        runtimeError("wrong arguments for: 'pop(array)'.");
        result.didError = true;
        result.value = NIL_VAL;
        return result;
    }
    ObjArray* array = AS_ARRAY(args[0]);
    int lastIdx = arrayGetLength(array) - 1;
    if (lastIdx < 0) {
        runtimeError("can't pop empty array.");
        result.didError = true;
        result.value = NIL_VAL;
        return result;
    }
    Value item = arrayReadFromIdx(array, lastIdx);
    arrayDeleteFrom(array, lastIdx);
    result.value = item;    // return the item pop'd
    return result;

}

// delete(array, index) - deletes entry from array - "delete(someArr, 99);" deletes entry on idx=99
static NativeResult arrDeleteNative(int argCount, Value* args) {
    NativeResult result;
    result.didError = false;
    if (argCount != 2 || !IS_ARRAY(args[0]) || !IS_NUMBER(args[1])) {
        runtimeError("wrong arguments for: 'delete(array, index)'.");
        result.didError = true;
        result.value = NIL_VAL;
        return result;
    }
    ObjArray* array = AS_ARRAY(args[0]);
    int idx = AS_NUMBER(args[1]);
    if ( !arrayIsValidIndex(array, idx)) {
        runtimeError("index out of bounds for: 'delete(array, index)'.");
        result.didError = true;
        result.value = NIL_VAL;
        return result;
    }
    arrayDeleteFrom(array, idx);
    result.value = NIL_VAL;
    return result;
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
    // to make lookup for "init()" we define this ObjString(string-interning):
    vm.initString = NULL;   // zero the field out to avoid GC reading undefined before copyString("init")
    vm.initString = copyString("init", 4);  
    // init Native Functions:
    defineNative("clock", clockNative);
    defineNative("push", arrPushNative);
    defineNative("pop", arrPopNative);
    defineNative("delete", arrDeleteNative);
    defineNative("len", lengthNative);
    defineNative("floor", floorNative);
    defineNative("printf", printfNative);
    defineNative("typeof", typeofNative);
    
}

void freeVM() {
    freeTable(&vm.globals);
    freeTable(&vm.strings);
    vm.initString = NULL;   // manually clear the pointer
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
            case OBJ_BOUND_METHOD: {
                ObjBoundMethod* bound = AS_BOUND_METHOD(callee);
                vm.stackTop[-argCount -1] = bound->receiver;    // the zero slot in the new callframe, we write our receiver in that slot
                return call(bound->method, argCount);   // we unwrap the method from the Boundmethod and call it
            }
            case OBJ_CLASS: {
                // To instance a Class we reuse callValue - instead of 'new SomeClass' we do 'SomeClass()'
                ObjClass* pClass = AS_CLASS(callee);
                vm.stackTop[-argCount -1] = OBJ_VAL(newInstance(pClass));
                // automatically call init on new instances: class Brunch{ init(food, count){}} \n Brunch("coffee", 2)
                Value initializer;
                if (tableGet(&pClass->methods, vm.initString, &initializer)) {
                    return call(AS_CLOSURE(initializer), argCount);     // this will make sure number of arguments match (like any fn call)
                } else if (argCount != 0) {
                    runtimeError("Expected 0 arguments but got %d.", argCount);   // if no init() -> cant pass in not 0 arguments in NewClass(1,2)
                    return false;
                }
                return true;
            }
            case OBJ_CLOSURE:
                return call(AS_CLOSURE(callee), argCount);
            case OBJ_NATIVE: {
                // if the object being called is a native function -> invoke the C-Function right there
                NativeFn native = AS_NATIVE(callee);
                NativeResult result = native(argCount, vm.stackTop - argCount);
                vm.stackTop -= argCount + 1;
                push(result.value);   // we use the result from the C-Function and stuff it back in the stack
                if (result.didError) return false;
                return true;
            }
            default:
                break;          // Non-callable object type tried to call ex.: "just string"()
        }
    }
    runtimeError("Can only call functions and classes.");
    return false;
}

// helper for invoke() - combines logic for OP_GET_PROPERTY and OP_CALL, but with less lookups/stack ready -> faster
// - lookup method by name in method-table. (error if not found)
// - take the moethods closure and push a call to in on the CallFrame stack. (receiver and method arguments are already there)
static bool invokeFromClass(ObjClass* pClass, ObjString* name, int argCount) {
    Value method;
    if (!tableGet(&pClass->methods, name, &method)) {
        runtimeError("Undefined property '%s'.", name->chars);
        return false;
    }
    return call(AS_CLOSURE(method), argCount);
}

// helper for run() - read receiver Instance from stack and pass that down to invokeFromClass
static bool invoke(ObjString* name, int argCount) {
    Value receiver = peek(argCount);                // read receiver from the stack (its below arguments on the stack)
    if (!IS_INSTANCE(receiver)) {
        runtimeError("Only instances have methods.");
        return false;
    }
    ObjInstance* instance = AS_INSTANCE(receiver);
    // fields get priority and shadow over methods -> so we look up a field first. (and fields can hold closures of functions and thus get called)
    Value value;
    if (tableGet(&instance->fields, name, &value)) {
        vm.stackTop[-argCount - 1] = value;
        return callValue(value, argCount);
    }

    return invokeFromClass(instance->pClass, name, argCount);
}

// helper for run() case OP_GET_PROPERTY - 
// 1. check for method with given name in pClass's method table (if not runtime-Error)
// 2. take the method and wrap it together with the Instance(pop() from stack)
// 3. last we push the ObjBoundMethod on the stack
static bool bindMethod(ObjClass* pClass, ObjString* name) {
    Value method;
    if (!tableGet(&pClass->methods, name, &method)) {
        runtimeError("Undefined property '%s'.", name->chars);
        return false;
    }
    ObjBoundMethod* bound = newBoundMethod(peek(0), AS_CLOSURE(method));
    pop();                      // pop the Instance from the stack
    push(OBJ_VAL(bound));
    return true;
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
                    return INTERPRET_RUNTIME_ERROR;
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
            case OP_INVOKE: {               // Method calls got their special Invoke OpCode to make those lookups faster
                ObjString* method = READ_STRING();
                int argCount = READ_BYTE();
                if (!invoke(method, argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frameCount - 1];
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
                // next we check if its a method instead, if neither we runtime error:
                if (!bindMethod(instance->pClass, name)) {
                    //runtimeError("Undefined property '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
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
            case OP_INHERIT: {
                Value superclass = peek(1);                 // 2nd on the stack
                if (!IS_CLASS(superclass)) {
                    runtimeError("Superclass must be a class.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjClass* subclass = AS_CLASS(peek(0));     // top on the stack
                tableAddAll(&AS_CLASS(superclass)->methods, &subclass->methods);    // copy method table -> inherit methods
                pop();
                break;
            }
            case OP_GET_SUPER: {
                // we resolve using the function and the pop'd superclass from top of stack
                // bindMethod skips over any overriding methods in any of the subclasses between that superclass and the owner
                ObjString* name = READ_STRING();
                ObjClass* superclass = AS_CLASS(pop());
                if (!bindMethod(superclass, name)) {
                    return INTERPRET_RUNTIME_ERROR;         // can only methods from superclass
                }
                break;
            }
            case OP_SUPER_INVOKE: {
                // the optimized way to invoke a super method (replace OP_GET_SUPER lookup and following OP_CALL)
                ObjString* method = READ_STRING();
                int argCount = READ_BYTE();
                ObjClass* superclass = AS_CLASS(pop());
                if (!invokeFromClass(superclass, method, argCount)) {
                    return INTERPRET_RUNTIME_ERROR; // if method is not found we abort
                }
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }
            case OP_METHOD:
                defineMethod(READ_STRING());
                break;
            
            /* CUSTOM OpCommands implemented ontop of the default lox */
            case OP_MAP_BUILD: {
                // stack at start: [key1:value1, key2:value2, keyN:valueN, count]top -> at end: [map]
                ObjMap* map = newMap();
                uint8_t pairsCount = READ_BYTE();    
                push(OBJ_VAL(map));                 // we push map so it doesnt GC'd
                for (int i = pairsCount*2; i>0; i-=2) {
                    ObjString* key = AS_STRING( peek(i) );
                    Value value = peek(i-1);
                    tableSet(&map->table, key, value);

                }
                // cleanup of stack: (map then all key-value-pairs)
                pop();
                for(int i=0; i<pairsCount; i++){
                    pop();
                    pop();
                }
                push(OBJ_VAL(map));
                break;

            }
            case OP_ARRAY_BUILD:{
                // stack at start: [item1, item2 ... itemN, count]top -> at end: [array]
                // takes operand of items and count = Nr. of values on the stack that fill the array
                ObjArray* array = newArray();
                uint8_t itemCount = READ_BYTE();    // count of following item-values waiting on stack
                push(OBJ_VAL(array));               // we push our array on the stack so it doesnt get removed by GC
                // fill our Array with items:
                for (int i = itemCount; i>0; i--) { // items are reverse order on the stack
                    arrayAppendAtEnd(array, peek(i));
                }
                pop();                              // remove array from stack that was only there for GC safety
                // Pop all items from the stack
                while (itemCount-- > 0) {
                    pop();
                }
                push(OBJ_VAL(array));               // stack at end: [array]
                break;
            }     
            case OP_LISTS_READ_IDX: {
                if (IS_MAP(peek(1))) {
                    /** It is a Map */
                    if (!IS_STRING(peek(0))) {
                        runtimeError("Map key must be a string.");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    ObjString* key = AS_STRING(pop()); // should be ok to pop here, since no allocation
                    ObjMap* map = AS_MAP(pop());
                    Value result;
                    bool isInMap = tableFindValue(&map->table, key->chars, key->length, key->hash, &result);
                    if (!isInMap) {
                        push(NIL_VAL);  // if we cant find in map we return NIL
                    } else {
                        push(result);   // if we found it we return reference to the Value
                    }
                    break;
                } else {
                    /** It is a Array */
                    // stack at start: [array, idx]top -> at end: [value*]
                    // takes operand [array, idx] -> reads value in Array on that index.
                    if (!IS_NUMBER(peek(0))) {
                        runtimeError("Array index must be a number.");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    int idx = AS_NUMBER(pop());
                    if (!IS_ARRAY(peek(0))) {
                        runtimeError("Can only index into an Array or Map.");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    ObjArray* array = AS_ARRAY(pop());
                    if(!arrayIsValidIndex(array, idx)) {
                        runtimeError("Array index=%d out of range. Current len()=%d.", idx, arrayGetLength(array));
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    Value result = arrayReadFromIdx(array, idx);
                    push(result);
                    break;
                }
            } 
            case OP_LISTS_WRITE_IDX: {
                if (IS_MAP(peek(2))) {
                    /** It is a Map */
                    Value value = peek(0);
                    if (!IS_STRING(peek(1))) {
                        runtimeError("Map key must be a string.");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    ObjString* key = AS_STRING(peek(1));
                    ObjMap* map = AS_MAP(peek(2));          // keeping value, key, map GC secure
                    // writing nil to a value == deleting in our implementation:
                    if ( IS_NIL(value)) {
                        tableDelete(&map->table, key);
                    } else {
                        tableSet(&map->table, key, value);
                    }
                    pop();          // we kept value on for GC
                    pop();
                    pop();  
                    push(value);    // in lox assignments return the assigned value
                    break;

                } else {
                    /** It is a Array */
                    // stack at start: [array, idx, value]top -> at end: [array]
                    // takes operand [array, idx, value] writes value to array at index:idx
                    Value item = peek(0);     // the value that should get added
                    if (!IS_NUMBER(peek(1))) {
                        runtimeError("Array index must be a number.");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    int idx = AS_NUMBER(peek(1));
                    if (!IS_ARRAY(peek(2))) {
                        runtimeError("Can not store value in a non-array/map.");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    ObjArray* array = AS_ARRAY(peek(2));
                    if (!arrayIsValidIndex(array, idx)) {
                        runtimeError("Invalid index to array.");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    arrayWriteTo(array, idx, item);
                    pop();      // we kept value on for GC
                    pop();
                    pop();
                    push(item); // in lox assignments return the assigned value -> x=3=x*3=[x,x*2];
                    break;
                }
            }
            case OP_MODULO:{
                if ( !IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1)) ) { 
                    runtimeError("Operands must be numbers."); 
                    return INTERPRET_RUNTIME_ERROR; 
                } 
                double b = AS_NUMBER(pop()); 
                double a = AS_NUMBER(pop());
                //double res = (int) a % (int) b; // this garbage will at least make % work for 'ints'
                double res = myFloatModulo(a, b);   // also not perfect (negatives wrong)
                push(NUMBER_VAL(res));          // but i really DONT want to use any libs.
            }
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