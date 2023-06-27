# Codealong for crafting interpreters - the clox Variant in c
The book Crafting Interpreters by Robert Nystrom as a guide, the goal is to dip my toes into writing a compiler/vm.
## Notes
### A Value Stack Manipulator - The VM's Stack
- the jlox interpreter accomplished by recursively traversing(postorder traversal) the AST.
- But since the interpret() run() function of the clox is flattened out, a place for tempory values is needed (the Value Stack Manipulator)

![value stack](./value_stack.excalidraw.svg)

- as shown in the above we can just make a stack for our values to store, that we push onto if we need a new temp store or pop off, as needed.