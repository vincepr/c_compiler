# Implementing my own Functionality on top of default-lox
## Arrays / List
Some kind of array/list type is definitly needed. The idea is to wrap the dynamic Array we used all over the place in lox. This should provide fixed time for accessing fields.

And be fairly quick to iterate over, since it should get allocated in a close block together, compared to a linked list implementation.

### Implementation in the runtime
- `object.h and object.c`
```c
typedef struct {
    Obj obj;
    int count;
    int capacity;
    Value* items;       // the array should take different kind of values, just like a JS-Array
} ObjList;

ObjArray* newArray() {
    ObjArray* array = ALLOCATE_OBJ(ObjArray, OBJ_ARRAY);
    array->items = NULL;
    array->count = 0;
    array->capacity = 0;
    return array;
}
```

- created new files `array.c and array.h`
```c
void arrayAppendAtEnd(ObjArray* array, Value value) {
    if (array->capacity < array->count + 1) {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->items = GROW_ARRAY(Value, array->items, oldCapacity, array->capacity);
    }
    array->items[array->count] = value;
    array->count++;
}
void arrayWriteTo(ObjArray* array, int index, Value value) {
    array->items[index] = value;
}
Value arrayReadFromIdx (ObjArray* array, int index) {
    return array->items[index];
}
void arrayDeleteFrom(ObjArray* array, int index) {
    for (int i = index; i < array->count -1; i++) {
        array->items[i] = array->items[i+1];
    }
    array->items[array->count-1] = NIL_VAL;
    array->count--;
}
bool arrayIsValidIndex(ObjArray* array, int index) {
    return (index >= 0 || index < array->count);
}

```


- `vm.c in run()`
```c
case OP_ARRAY_BUILD:{
    // stack at start: [item1, item2 ... itemN, count]top -> at end: [array]
    // takes operand of items and count = Nr. of values on the stack that fill the array
    ObjArray* array = newArray();
    uint8_t itemCount = READ_BYTE();    // count of following item-values waiting on stack
    push(OBJ_VAL(array));               // we push our array on the stack so it doesnt get removed by GC
    // fill our Array with items:
    for (int i = itemCount; i>0; i--) { // items are reverse order on the stack
        arrayAppendAtEnd(array, peek(i));
    }
    pop();                              // remove array from stack that was only there for GC safety
    // Pop all items from the stack
    while (itemCount-- > 0) {
        pop();
    }
    push(OBJ_VAL(array));               // stack at end: [array]
    break;
}     
case OP_ARRAY_READ_IDX: {
    // stack at start: [array, idx]top -> at end: [value*]
    // takes operand [array, idx] -> reads value in Array on that index.
    Value result;
    if (!IS_NUMBER(peek(0))) {
        runtimeError("Array index must be a number.");
        return INTERPRET_RUNTIME_ERROR;
    }
    int idx = AS_NUMBER(pop());
    if (!IS_ARRAY(peek(0))) {
        runtimeError("Can only index into an array.");
        return INTERPRET_RUNTIME_ERROR;
    }
    ObjArray* array = AS_ARRAY(pop());
    if(!arrayIsValidIndex(array, idx)) {
        runtimeError("Array index out of range.");
        return INTERPRET_RUNTIME_ERROR;
    }
    result = arrayReadFromIdx(array, idx);      //TODO: check if this must be AS_NUMBER(idx)
    push(result);
    break;
} 
case OP_ARRAY_WRITE: {
    // stack at start: [array, idx, value]top -> at end: [array]
    // takes operand [array, idx, value] writes value to array at index:idx
    Value item = pop();     // the value that should get added
    if (!IS_NUMBER(peek(0))); {
        runtimeError("Array index must be a number.");
        return INTERPRET_RUNTIME_ERROR;
    }
    int idx = AS_NUMBER(pop());
    if (!IS_ARRAY(peek(0))) {
        runtimeError("Can not store value in a non-array.");
        return INTERPRET_RUNTIME_ERROR;
    }
    ObjArray* array = AS_ARRAY(pop());
    if (!arrayIsValidIndex(array, idx)) {
        runtimeError("Invalid index to array.");
        return INTERPRET_RUNTIME_ERROR;
    }
    arrayWriteTo(array, idx, item);
    push(item);
    break;
}
```

### Implementation in the scanner and compiler
- implemented `TOKEN_LEFT_BRACKET and TOKEN_RIGHT_BRACKET` in the scanner
- then added the following precedence:
```c
    [TOKEN_LEFT_BRACKET]  = {arrayInit,   arrayEdit, PREC_IDX_ARRAY},
    [TOKEN_RIGHT_BRACKET] = {NULL,        NULL,      PREC_NONE},
```

- and hook up the used functions
```c
// parsing function for array initializations
// - we just parse everything separated by ',' push those values on stack
// - then we push the OpCode to build the array then the count 
static void arrayInit(bool canAssign) {
    int itemCount = 0;
    if (!check(TOKEN_RIGHT_BRACKET)) {
        do {
            if (check(TOKEN_RIGHT_BRACKET)){
                break;  // hit a trailing comma
            }
            parsePrecedence(PREC_OR);   // parses things between ','s and push values on stack
            if (itemCount == UINT8_COUNT) {
                error("Cant have more than 256 items in array.");
            }
            itemCount ++;
        } while (match(TOKEN_COMMA));
    }
    consume (TOKEN_RIGHT_BRACKET, "Expect ']' after array initialisation");
    emitByte(OP_ARRAY_BUILD);
    emitByte(itemCount);
}

// parsing function for array insertArr[idx] or assigning assignArr[idx] = true;
static void arrayEdit(bool canAssign) {
    parsePrecedence(PREC_OR);   // opening '[' already consumed so we expect the index next.
    consume(TOKEN_RIGHT_BRACKET, "Expect ']' after index.");
    if (canAssign & match(TOKEN_EQUAL)) {
        expression();                   // need to push the value that we gonna insert on the stack
        emitByte(OP_ARRAY_WRITE);       // were writing into the array ex: 'someArr[10] = true'
    } else {
        emitByte(OP_ARRAY_READ_IDX);    // were reading the value ex: 'someArr[10]'
    }
}
```

- extra functionailty, enabled with native functions: `vm.c`
```c
// ads push functionality to array: - "arrayPush(someArr, "insert this str"); "
static Value arrPushNative(int argCount, Value* args) {
    if (argCount != 2 || !IS_ARRAY(args[0])) {
        // TODO: handle runtime error
    }
    ObjArray* array = AS_ARRAY(args[0]);
    Value item = args[1];
    arrayAppendAtEnd(array, item);
    return NIL_VAL;
}

// deletes entry from array - "arrayDelete(someArr, 99);"
static Value arrDeleteNative(int argCount, Value* args) {
    if (argCount != 2 || !IS_ARRAY(args[0]) || !IS_NUMBER(args[1])) {
        //TODO: handle error
    }
    ObjArray* array = AS_ARRAY(args[0]);
    int idx = AS_NUMBER(args[1]);
    if ( !arrayIsValidIndex(array, idx)) {
        // TODO: handle error
    }
    arrayDeleteFrom(array, idx);
    return NIL_VAL;
}
```
### Hook up the GC
- `memory.c`
```c
// in blackenObject():
case OBJ_ARRAY: {
    ObjArray* array = (ObjArray*)object;
    for (int i=0; i<array->count; i++) {
        markValue(array->items[i]);
    }
    break;
}
// and in freeObject():
case OBJ_ARRAY: {
    ObjArray* array = (ObjArray*)object;
    for (int i=0; i<array->count; i++) {
        markValue(array->items[i]);
    }
    break;
}
```

## hooked up error handling to native Functions 