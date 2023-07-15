# lox compiler in C
this is a compiler & bytecode VM inclusive Garbage Collection, written in C without libraries.

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
- build the binary and run the test.lox file `make start`
- run the unit-testing suite: `make test`
- building for the web-browser: `build web` (this needs emcc from emscripten installed to compile c to a `.wasm` file). Afterwards just host the `./build_wasm` folder with something like life-server.

## The Lox Language
For the Lox Language itself you can refer to the book: https://craftinginterpreters.com/the-lox-language.html

### Changes from Lox
I decided to stay 100% lox compliant with the implementation in the book and just build on top of it.
- added dynamic Arrays
```js
var arr = [123, false, ];
arr = ["jimes", "bond" , arr];
arr[0] = "james";
print arr;              //-> ["james", "bond", [123, false, ]]

// push, pop and delete:
delete(arr, 0);         // delete by index
push(arr, false);       // pushes false to top
print pop(arr);         // prints false
```
#### added `len()` returns array length or chars in a string:
```js
print len("one");       // 3
print len([1,2,3,4]);   // 4
```
#### added modulo (though my i decided to use no libarires, so had to implement some scrappy modulo for doubles myself)
```js
print floor(12.9)       // 12 - rounds down
print 11%3              // 2 - takes modulo
```
#### implemented `printf(..args)` that takes in any amount of values and prints them without adding newline or whitespace:
```js
printf("I am Bob and my Age is: ", 22, " !")
```
#### implemented String Escaping with `\`. You can Escape `\\ \" \n \t`
```js
var str = "\tThis is a normal \"string\".\n"
```
#### added `typeof()` for runtime typechecking for the dynamic variables in lox
```js
print typeof("bob");                // "string"    
print typeof(12.3) == "number";     // true
Class Chicken{}
chick = Chicken();
print typeof(chick) == "Chicken";   // true
```
#### added dynamic Map/Dictionary
```js
var screen = {
    "length": 12.4,
    "width" : "12 meters",
};
screen["length"] = 77;    // changes length value
print screen["size"];     // -> nil    for not found
screen["width"] = nil;    // set value nil to delete from map
screen["height"] = "big"; // adds new key-value pair
print screen;             // -> { height : big, length : 77, }
```

some notes i took while implementing custom changes: [Notes while doing Custom Changes](https://github.com/vincepr/c_compiler/blob/b4a1ff81b5c3f5c4ae6313e0b5ba775d4ee93c5a/docs/CUSTOM_IMPLEMENTATIONS.md)

## Notes I took while coding along the chapters:
[Notes while doing the book](https://github.com/vincepr/c_compiler/blob/b4a1ff81b5c3f5c4ae6313e0b5ba775d4ee93c5a/docs/NOTES.md)