# Clox
Clox is a compiler & bytecode VM for lox in pure C, no libraries used. 

This is my implementation of Robert Nystrom's Crafting Interpreters, an awesome introductory Book into the World of Lexers, Compilers and Interpreters. 

## Web-Version
I made a wasm build that runs in your browser: 
- https://vincepr.github.io/c_compiler/


## Building yyourself
Just clone the repo, cd into it and if on linux (with gcc installed) you can just use my makefile: 
- for the repl: `make run`
- building the binary: `make build`
- building for the web-browser: `build web` (this needs emcc from emscripten installed to compile c to a `.wasm` file).

## The Language itself
For the Lox language itself you can refer to the book itself: https://craftinginterpreters.com/the-lox-language.html
