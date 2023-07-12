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


## Building with gcc or emscripten
Just clone the repo, cd into it and if on linux (with gcc installed) you can just use my makefile: 
- building the binary: `make build`
- for the repl: `make run`
- build the binary and run the test.lox file `make test`
- building for the web-browser: `build web` (this needs emcc from emscripten installed to compile c to a `.wasm` file).

## The Lox Language
For the Lox Language itself you can refer to the book: https://craftinginterpreters.com/the-lox-language.html

### Changes from Lox
I decided to stay 100% lox compliant with the implementation in the book and just build on top of it.
- added dynamic Arrays
```js
var arr = [123, false, ];
arr = ["jimes", "bond" , arr];
arr[0] = "james";
print arr;      //-> ["james", "bond", [123, false, ]]

// push, pop and delete:
delete(arr, 0);     // delete by index
push(arr, false);   // pushes false to top
print pop(arr);     // prints false -- pop's top element
```
- added modulo (though my i decided to use no libarires, so had to implement some scrappy modulo for doubles myself)
```js
print floor(12.9)         // 12 - rounds down
print 11%3                // 2 - takes modulo
```
- implemented `printf(..args)` that takes in any amount of values and prints them without adding newline or whitespace:
```js
printf("I am Bob and my Age is: ", 22, " !")
```

- some notes i took while implementing custom changes: https://github.com/vincepr/c_compiler/docs/CUSTOM_IMPLEMENTATIONS.md
## Notes I took while coding along the chapters:
https://github.com/vincepr/c_compiler/docs/NOTES.md