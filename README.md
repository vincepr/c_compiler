# Codealong for crafting interpreters - the clox Variant in c
The book Crafting Interpreters by Robert Nystrom as a guide, the goal is to dip my toes into writing a bytecode-compiler/vm.


## Notes arround implementation details (mostly for my later self)
### A Value Stack Manipulator - The VM's Stack
- the jlox interpreter accomplished by recursively traversing(postorder traversal) the AST.
- But since the interpret() run() function of the clox is flattened out, a place for tempory values is needed (the Value Stack Manipulator)

![value stack](./value_stack.excalidraw.svg)

- as shown in the above we can just make a stack for our values to store, that we push onto if we need a new temp store or pop off, as needed.

### String Interning
- The Problem: string comparison (if they are not the same instance/pointer) is slow.

String Interning is a process of deduplication. So textually identical strings will use the same underlying collection of data.

- This makes checking for string equality trivial. Since they would re-use the same original data.


We implement this by storing all active strings in Hash Table. (located in the vm-module `vm.c`)

#### Why this had to be done
- The cost of creating strings (additional interning happens here) has risen.

- BUT in return at runtime the equality operator runs way way faster. (since it just compares 2 pointers now!)
    - This is a must for our object dynamically-typed/oriented lox. Since method calls, or fields on classes are looked up as string identified identifiers/names at runtime.
    - so if those are slow everything will run slow.

### (global) Variables
jlox implemented variables by building a chain of environments, one for each scope. This will create a new hash table each time you change scope. So its to slow.

#### general info about variables in lox
**Global variables** in Lox are 'late bound' (aka resolved after compile time), or resolved dynamically. This means a function can be compiled, that uses a variable that gets declared later on. As long as the code does not **execute** before that definition happens.

**Local variables** on the other hand always get declared before beeing used. Thus our single pass compiler can resolve them at run time more easily.

#### Assignment
- The problem below is, the single-pass compiler, does not know that this is assignment, till it reaches the `=`
- By that point the compiler already has emitted bytecode for the whole thing up to it.
```
menu.brunch(sunday).beverage = "hello world";
```
- Variables only need a single identifier (before the `=`)
- The idea is, that **before** compiling an expression that could be as an assignment, we peek ahead for an `=`.
    - if we find a `=` we treat is as assignment or setter.
    - if not we compile it

### (local) Variables
They are not late bound, so different from globals, they need to be assigned before use.
- They are lexical scoped.
- for performance reasons we can just push/pop them on the stack. We can use the same stack we use to evaluate expressions.

![shows_local_scope_of_variables](./local_scope.excalidraw.svg)
- as seen above, all we need is keep track of the **offset**, to identify where on the stack each value is (ex `a`  after line 6, would need an offset of 2 down from the top).

