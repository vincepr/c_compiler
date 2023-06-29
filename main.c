#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

// usual repl behavior, read a line as input then interpret it
static void runRepl() {
  char line[1024];
  for (;;) {
    printf("> ");

    if (!fgets(line, sizeof(line), stdin)) {
      printf("\n");
      break;
    }

    interpret(line);
  }
}

// helper for runFile - Manages allocation and file-read things. Gets array of chars of the file basically
static char* readFile(const char* path) {
  FILE* file = fopen(path, "rb"); // open file
  // error handling - if opening file fails:
  if (file == NULL) {
    fprintf(stderr, "Could not open file \"%s\".\n", path);
    exit(74);
  }

  fseek(file, 0L, SEEK_END);      // find EndOfFile
  size_t fileSize = ftell(file);  // get size (from start to EndOfFile) in bytes
  rewind(file);                   // get back to start of file again

  char* buffer = (char*)malloc(fileSize + 1);   // allocate string of fitting size (filesize+1 for EOF='\0')
  // error handling - ran out of memory 
  if (buffer == NULL) {
    fprintf(stderr, "Not enough memory to read file \"%s\".\n", path);
    exit(74);
  }
  size_t bytesRead = fread(buffer, sizeof(char), fileSize, file); // read chars to string
  // error handling - ran out of memory 
  if (bytesRead < fileSize) {
    fprintf(stderr, "Could not read file \"%s\".\n", path);
    exit(74);
  }
  buffer[bytesRead] = '\0';                                       // add EoF to our 'string'

  fclose(file);                   // close file
  return buffer;
}

// other entrypoint than repl, but from a file.
static void runFile(const char* path) {
  char* source = readFile(path);
  InterpretResult result = interpret(source);
  free(source);

  if (result == INTERPRET_COMPILE_ERROR) exit(65);
  if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

int main(int argc, const char* argv[]) {
    printf("Compiler Running:\n");

    // initialize our VM:
    initVM();

    // Run either the REPL or OPEN-FILE or ERROR-MSG about wrong use.
    if (argc == 1) {
      runRepl();
    } else if (argc == 2) {
      runFile(argv[1]);
    } else {
      fprintf(stderr, "Usage: clox [path]\n");
      exit(64);
    }

    // free the VM
    freeVM();
    return 0;
}

