#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <emscripten/emscripten.h>

#include "src/common.h"
#include "src/chunk.h"
#include "src/debug.h"
#include "src/vm.h"

#ifdef __cplusplus
#define EXTERN extern "C"
#else
#define EXTERN
#endif

// EXTERN EMSCRIPTEN_KEEPALIVE 
// void compileSourceCode(int argc, char ** argv) {
//     printf("MyFunction Called\n");
// }

char* compileSourceCode(char* x) {
    printf("its running;");
    return "world";
}
/*
compiled via:
emcc main-web.c src/chunk.c src/memory.c src/debug.c src/value.c src/vm.c src/compiler.c src/scanner.c src/object.c src/table.c -o build_wasm/compiler.html -sEXPORTED_FUNCTIONS=_compileSourceCode -sEXPORTED_RUNTIME_METHODS=ccall,cwrap

in js:
compileSourceCode = Module.cwrap('compileSourceCode', 'string', ['string'])
compileSourceCode("hello")

*/


// // other entrypoint than repl, but from a file.
// static void runFile(const char* sourceCode) {
//     printf("Compiler Running:\n");

//     // initialize our VM:
//     initVM();

// 	InterpretResult result = interpret(sourceCode);
// 	free(sourceCode);

// 	if (result == INTERPRET_COMPILE_ERROR) exit(65);
// 	if (result == INTERPRET_RUNTIME_ERROR) exit(70);

//     // free the VM
//     freeVM();
// }


