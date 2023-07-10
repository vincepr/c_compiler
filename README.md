# lox compiler in C
this is a compiler & bytecode VM for lox in pure C, no libraries used. 

This is my implementation of Robert Nystrom's Crafting Interpreters, an awesome introductory Book into the World of Lexers, Compilers and Interpreters. 

## Web-Version
I made a wasm build that runs in your browser.

- https://vincepr.github.io/c_compiler/

It uses the monaco-editor, the same Editor that powers VS-Code as frontend and some plain Javascript, html and CSS as Glue.

You can toggle 3 Flags in the web version:

- `bytecode` this sets the flag: `FLAG_PRINT_BYTECODE` 
    - set this to display the Bytecode-Instructions the source code gets compiled down to.
- `execution` this sets the flag: `FLAG_TRACE_EXECUTION` 
    - set this to display all the Instructions as the VM runs trough them when executing.
- `log GC` this sets the flag: `FLAG_LOG_GC` 
    - display all Garbage Collection, Allocation, and Freeing of Memory caused by the code-execution.


## Building yourself
Just clone the repo, cd into it and if on linux (with gcc installed) you can just use my makefile: 
- for the repl: `make run`
- building the binary: `make build`
- building for the web-browser: `build web` (this needs emcc from emscripten installed to compile c to a `.wasm` file).

## The Lox Language
For the Lox Language itself you can refer to the book: https://craftinginterpreters.com/the-lox-language.html

## Notes I took while coding along the chapters:
https://github.com/vincepr/c_compiler/docs/NOTES.md