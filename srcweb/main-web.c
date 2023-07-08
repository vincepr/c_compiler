#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <emscripten/emscripten.h>


#include "../src/common.h"
#include "../src/chunk.h"
#include "../src/debug.h"
#include "../src/vm.h"

//#undef DEBUG_PRINT_CODE         // comment this out: to enable debug printing
//#undef DEBUG_TRACE_EXECUTION    // comment this out: to enable trace-execution
//#undef DEBUG_LOG_GC             // comment this out: to enable loging of GC steps
#undef DEBUG_STRESS_GC          // comment this out: to enable GC every step

// we define needed Flags:
#ifdef DEBUG_PRINT_CODE
bool FLAG_PRINT_CODE = false;
#endif

#ifdef DEBUG_TRACE_EXECUTION
bool FLAG_TRACE_EXECUTION = false;
#endif

#ifdef DEBUG_LOG_GC
bool FLAG_LOG_GC = false;
#endif


EMSCRIPTEN_KEEPALIVE 
int runCompiler(char* sourceCode, bool isBytecode, bool isTrace, bool isGc) {
    // we pass down optional-flags set in frontend:
    #ifdef DEBUG_PRINT_CODE
    FLAG_PRINT_CODE = isBytecode;
    if (FLAG_PRINT_CODE ) printf("FLAG_PRINT_BYTECODE is ON \n");
    #endif
    #ifdef DEBUG_TRACE_EXECUTION
    FLAG_TRACE_EXECUTION = isTrace;
    if (FLAG_TRACE_EXECUTION ) printf("FLAG_TRACE_EXECUTION is ON \n");
    #endif
    #ifdef DEBUG_LOG_GC
    FLAG_LOG_GC = isGc;
    if (FLAG_LOG_GC ) printf("FLAG_LOG_GC is ON \n");
    #endif

    
    printf(" -- Compiling code: --\n\n");

    // ido the compiling:
    initVM();
	InterpretResult result = interpret(sourceCode);
	free(sourceCode);
    // free the VM
    freeVM();
    if (result == INTERPRET_COMPILE_ERROR) return 65;   // return/exit with 65 -> compitle error
	if (result == INTERPRET_RUNTIME_ERROR) return 70;   // return/exit with 70 -> runtime  error
    return 0;                                           // return/exit with 0  -> success
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


