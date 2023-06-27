#include "common.h"
#include "debug.h"
#include "vm.h"
#include <stdio.h>

// instance of our VM:
VM vm;

// helperFunction to setup/reset the stack
static void resetStack() {
    vm.stackTop = vm.stack;     // we just reuse the stack. So we can just point to its start
}

void initVM() {
    resetStack();
}

void freeVM() {

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


// helper function for interpret() that actually runs the current instruciton
static InterpretResult run() {
// READ_BYTE-macro reads the byte currently pointed at by the instruction-pointer(ip) then advances the ip.
#define READ_BYTE() (*vm.ip++)
// READ_CONSTANT-macro: reads the next byte from the bytecoat, treats it number as index and looks it up in our constant-pool
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])

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
            // OP_CONSTANT - 
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push(constant);             // push the constant/tempory-value to the stack
                break;
            }
            // OP_RETURN - exits the loop entirely (end of chunk reached/return from the current Lox function)
            case OP_RETURN: {
                printValue(pop());      // "produce a value from the stack", for now we just print it out.
                printf("\n");
                return INTERPRET_OK;
            }
        }
    }

// we only need our macros in run() so we scope them explicity to only be available here:
#undef READ_BYTE
#undef READ_CONSTANT
}

// tell the vm to runn the input-chunk of instructions
InterpretResult interpret(Chunk* chunk) {
    vm.chunk = chunk;       // set as currently active chunk
    vm.ip = vm.chunk->code; // pointer to the current idx of the instruction were working on
    return run();           // actually runs the bytecode instructions
}


