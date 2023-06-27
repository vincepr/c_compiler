#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"
#include <stdio.h>

int main(int argc, const char* argv[]) {
    printf("Compiler Running:\n");

    // initialize our VM:
    initVM();

    // do the repl or open file or error msg about use.
    if (argc == 1) {
      repl();
    } else if (argc == 2) {
      runFile(argv[1]);
    } else {
      printf(stderr, "Usage: clox [path]\n");
      exit(64);
    }

    // free the VM
    freeVM();
    return 0;
}