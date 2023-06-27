#include "common.h"
#include "vm.h"
#include <stdio.h>

// instance of our VM:
VM vm;

void initVM() {

}

void freeVM() {

}

// helper function for interpret() that actually runs the current instruciton
static InterpretResult run() {
// READ_BYTE-macro reads the byte currently pointed at by the instruction-pointer(ip) then advances the ip.
#define READ_BYTE() (*vm.ip++)
// READ_CONSTANT-macro: reads the next byte from the bytecoat, treats it number as index and looks it up in our constant-pool
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])

    for(;;) {
        // first byte of each instruction is opcode so we decode/dispatch it:
        uint8_t instruciton;
        switch (instruciton = READ_BYTE()) {
            // OP_CONSTANT - 
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                printValue(constant);
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
}

// tell the vm to runn the input-chunk of instructions
InterpretResult interpret(Chunk* chunk) {
    vm.chunk = chunk;       // set as currently active chunk
    vm.ip = vm.chunk->code; // pointer to the current idx of the instruction were working on
    return run();           // actually runs the bytecode instructions
}

