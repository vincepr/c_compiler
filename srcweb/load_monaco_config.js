
// String that stores our 'preview'-files
const allvalues = {
    class_lox:  `//
// The Basics about Classes in Lox
//

class Person {
  // only methods can be declared here (no fields)
  init(name) {
    this.name = name; // this is how initial fields could be set
    this.type = "human";
  }
  whoAmI() {
    print "Hello, my name is " + this.name + " and i am " + this.type;
  }
}

// creating instances and calling methods:
var p1 = Person("Bob");
var p2 = Person("James");
print p1.name;
p2.whoAmI();

// inheritance is possible:
class German < Person {
  whoAmI() {
    super.whoAmI();
    print "and i am german.";
  }
}

var p3 = German("Ute");
p3.whoAmI();

// it is also possible to add fields as needed:
p2.checksum = "#12312d#";
print p2.checksum;`,




    fib_lox: `// 
// calculate a fibonacci-nr and return the time it took in seconds:
//

fun fib(n) {
  if (n < 2) return n;
  return fib(n - 2) + fib(n - 1);
}

var start = clock();
var n = 31;
printf("fibonnacci Nr of ", n, " is ", fib(n) );    // prints multiple arguments, no newline.
print " ";                                          // print always adds a newline in lox.
printf("time spent running in seconds: ");
print clock() - start;
`,




    closure_lox:  `// 
// Closures are working as they would in Javascript
//

var x = "global";
fun outer() {
  var x = "outer";
  fun inner() {
    print x;
  }
  inner();
}
outer();    // closure captured "outer" and should print that


// closures here can capture the same outer variable inc.\n
fun incrementClosure() {
  var counter = 100;
  fun increment() {
    counter = counter +10;
    print counter;
  }
  return increment;
}
var a = incrementClosure();
var b = incrementClosure();
a();  // 110
a();  // 120 this captures the same counter as the above
b();  // 110 this closes over its own counter (compared to a)
b();  // 120 same inc as above
b();  // 130 same inc as above
`,




    error_lox: `//
//  Lox can Runtime-Error or Compiletime-Error
//

// Compile Time Errors like bad parsing:
print "H\\nE\\nL\\nL\\nO\\n\\n" = ;       // try removing the '='

// RuntimeErrors happen at Compile time, and Code till the error gets executed;
var x = 123 + "somesting";      // try fixing this error.

class Person {
init(name) {
  this.name = name;
}
whoAmI() {
  printf( "Hello, my name is " + this.name + " ");
}
}

class Parent < Person { 
  init(name, children) {
    super.init(name);
    this.children = children;
  }
}

// Functions are Firstclass in Lox 
fun fnPerson(idx) {
  household[idx].whoAmI();
}

fun fnParent(idx) {
  household[idx].whoAmI();
  var children = household[idx].children;
  printf("and my children are: ");
  for (var i=0; i<len(children); i=i+1){
    printf(children[i], " ");
  }
}

fun fnCat() {
  printf("I am the Family's Cat");
}

fun fnDog() {
  printf("And who let the dogs out? Woof Woof.");
}

// so we can put a bunch of closures in A Map:
var map = {
  "Person" : fnPerson,
  "Dog" : fnDog,
};
map["Cat"] = fnCat;
map["Parent"] = fnParent;

var p1 = Person("Bob");
var p2 = Person("James");
var p3 = Person("James");
var p4 = Parent("Jenny", ["Bob", "James", "Finn"]);
var household = [p1, p2, "Cat" , p4, p3, "Dog"];

for (var i=0; i<len(household); i=i+1) {
  // typechecking happens at runtime, so we can branch with if:
  if (typeof(household[i])=="string") {
    map[household[i]]();
  }else {
    // or use a map like above to branch over for example classes:
    var fn = map[typeof(household[i])];
    fn(i);
  }
  printf("\n");
}
`,




    arrays_lox: `// 
//I added Arrays (more a list like dynamic array) ontop of the default Lox-Language:
//

// you can initialize arrays like this:
var arr = ["bond", "james"];
print arr;
// like any dynamic variable we can reasign them as we wish (GC will handle the rest):
arr = [1, 2, "world", false, arr];
print arr;

// we can directly index into an array or assign:
print arr[4][0];
print arr[2];
arr[0] = "____ ____"; 

// you can use len(arr) or len("somestring") to get the length
for (var i=0; i<len(arr); i=i+1) {
  //print i;
  print arr[i];
}
// then there is push(array, value)   and   pop(arary) = vale
var arr = [];
var i = 0;
while (i<10) {
  i = i + 1;
  push(arr, i*100);
}
print arr;
while (i>=0) {
  i = i - 2;
  print pop(arr);
}
print arr;
// and delete(array, index) to delete at a certain index:
delete(arr, 1);
print arr;
`,



    custom_lox: `// 
//  Custom Implementations on top of default-Lox (without breaking any behavior)
//

// added String Escape
var stringEscape = "we can escape \\\\ \\" \\n \\t as expected.\\n";
print stringEscape;


// added printf(), since print forces newline and only takes 1 argument
var x = "prtinf() combines all arguments, ";
var y = "without adding any newlines\\n automatically";
printf(x, y, "\\t",true, ["even arrays", 101,], "bye\\n\\n");


// added Arrays themselfs, push(), pop(), delete() for arrays:
var arr = [];
for (var i=0; i<100; i=i+1){
  push(arr, i*10);
}
// added len(arr); and len("some string");
for (var idx=len(arr)-1; idx>=1; idx=idx-2) {
  delete(arr, idx);
}
printf("array of 20x's:\\n", arr, "\\n\\n");


// runtime typechecking with typeof
print typeof("bob");            // "string
var xyz = 12.45;
print typeof(xyz) == "number";  // true

// Instances of Classes can typecheck for the "Classname"
class Chicken {
  init(color) {
    if (typeof(color)=="string" ) this.color = color;
    else this.color = "default";
  }
}
var blueChick = Chicken("blue");
var anotherChick = Chicken(1);
if (typeof(blueChick)=="Chicken"){
  print blueChick.color;        // blue
}
print anotherChick.color;       // default
print "\\n";


// Math operations  
print floor(12.9);              // 12 - rounds down
print 11%3;                     // 2 - Modulo Operator

// Map/Dictionary
var screen = {
  "length": 12.4,
  "width" : "12 meters",
};
screen["height"] = "big";
screen["length"] = 77;
print screen["size"];         // nil
screen["width"] = nil;        // Delete from Map
print screen;                 // { height : big, length : 77, }
`,
};


/* Custom Editor Theme */
const ace_theme_monaco = {
    base: 'vs',
    inherit: true,
    rules: [
        { token: '', foreground: '5c6773' },
        { token: 'invalid', foreground: 'ff3333' },
        { token: 'emphasis', fontStyle: 'italic' },
        { token: 'strong', fontStyle: 'bold' },
        { token: 'variable', foreground: '5c6773' },
        { token: 'variable.predefined', foreground: '5c6773' },
        { token: 'constant', foreground: 'f08c36' },
        { token: 'comment', foreground: '86b300'},
        { token: 'number', foreground: '3777bd' },
        { token: 'number.hex', foreground: '3777bd' },
        { token: 'regexp', foreground: '4dbf99' },
        { token: 'annotation', foreground: '41a6d9' },
        { token: 'type', foreground: '96580c' },
        { token: 'delimiter', foreground: '5c6773' },
        { token: 'delimiter.html', foreground: '5c6773' },
        { token: 'delimiter.xml', foreground: '5c6773' },
        { token: 'tag', foreground: 'e7c547' },
        { token: 'tag.id.jade', foreground: 'e7c547' },
        { token: 'tag.class.jade', foreground: 'e7c547' },
        { token: 'meta.scss', foreground: 'e7c547' },
        { token: 'metatag', foreground: 'e7c547' },
        { token: 'metatag.content.html', foreground: '86b300' },
        { token: 'metatag.html', foreground: 'e7c547' },
        { token: 'metatag.xml', foreground: 'e7c547' },
        { token: 'metatag.php', fontStyle: 'bold' },
        { token: 'key', foreground: '41a6d9' },
        { token: 'string.key.json', foreground: '41a6d9' },
        { token: 'string.value.json', foreground: '3777bd' },
        { token: 'attribute.name', foreground: 'f08c36' },
        { token: 'attribute.value', foreground: '0451A5' },
        { token: 'attribute.value.number', foreground: 'abb0b6' },
        { token: 'attribute.value.unit', foreground: '86b300' },
        { token: 'attribute.value.html', foreground: '86b300' },
        { token: 'attribute.value.xml', foreground: '86b300' },
        { token: 'string', foreground: '3777bd' },
        { token: 'string.html', foreground: '3777bd' },
        { token: 'string.sql', foreground: '3777bd' },
        { token: 'string.yaml', foreground: '3777bd' },
        { token: 'keyword', foreground: 'f2590c' },
        { token: 'keyword.json', foreground: 'f2590c' },
        { token: 'keyword.flow', foreground: 'f2590c' },
        { token: 'keyword.flow.scss', foreground: 'f2590c' },
        { token: 'operator.scss', foreground: '666666' }, //
        { token: 'operator.sql', foreground: '778899' }, //
        { token: 'operator.swift', foreground: '666666' }, //
        { token: 'predefined.sql', foreground: 'FF00FF' }, //
    ],
    colors: {
        'editor.background': '#fafafa',
        'editor.foreground': '#5c6773',
        'editorIndentGuide.background': '#ecebec',
        'editorIndentGuide.activeBackground': '#e0e0e0',
    },
};


/* custom lox-syntax highlighting: */
const makeTokensProvider = () => {
    return {
        keywords: [
            "and",
            "break",
            "class",
            "else",
            "false",
            "for",
            "fun",
            "if",
            "init",
            "let",
            "nil",
            "or",
            "print",
            "return",
            "super",
            "this",
            "true",
            "var",
            "while",
            // my custom implementations:
            "printf",
            "floor",
            "typeof",
            "clock",
            "push",
            "pop",
            "delete"
        ],

        operators: [
            ":",
            "-",
            "+",
            "/",
            "*",
            "!",
            "!=",
            "=",
            "==",
            ">",
            ">=",
            "<",
            "<=",
            ",",
            "%",
        ],

        symbols: /[=><!:+\-*\/,%]+/,
        escapes: /\\(?:[abfnrtv\\"']|x[0-9A-Fa-f]{1,4}|u[0-9A-Fa-f]{4}|U[0-9A-Fa-f]{8})/,
        digits: /\d+(_+\d+)*/,

        tokenizer: {
            root: [
                // identifiers and keywords
                [
                    /[a-z_$][\w$]*/,
                    { cases: { "@keywords": "keyword", "@default": "identifier" } },
                ],
                [/[A-Z][\w\$]*/, "type.identifier"], // to show class names nicely
        
                // whitespace
                { include: "@whitespace" },
        
                // delimiters and operators
                [/[{}()]/, "@brackets"],
                [/@symbols/, { cases: { "@operators": "operators", "@default": "" } }],
        
                // numbers
                [/\d*\.\d+/, "number.float"],
                [/\d+/, "number"],
        
                // delimiter: after number because of .\d floats
                [/[;,.]/, "delimiter"],
        
                // strings
                [/"([^"\\]|\\.)*$/, "string.invalid"], // non-teminated string
                [/"/, 'string', '@string_double'],
                [/"/, { token: "string.quote", bracket: "@open", next: "@string" }],
            ],
    
            string: [
                [/[^\\"]+/, "string"],
                [/"/, { token: "string.quote", bracket: "@close", next: "@pop" }],
            ],
    
            whitespace: [
                [/[ \t\r\n]+/, "white"],
                [/\/\*/, "comment", "@comment"],
                [/\/\/.*$/, "comment"],
            ],
    
            comment: [
                [/[^\/*]+/, "comment"],
                [/\/\*/, "comment", "@push"], // nested comment
                ["\\*/", "comment", "@pop"],
                [/[\/*]/, "comment"],
            ],
            // we match regular expression quite precisely
            string_double: [
              [/[^\\"]+/, 'string'],
              [/@escapes/, 'string.escape'],
              [/\\./, 'string.escape.invalid'],
              [/"/, 'string', '@pop']
            ],
        
        },
    };
};

/* needed this additionally to enable proper bracket behavior, toggle comments etc.. */
const lox_lang_config = {
    //wordPattern: /(-?\d*\.\d\w*)|([^\`\~\!\@\#\%\^\&\*\(\)\-\=\+\[\{\]\}\\\|\;\:\'\"\,\.\<\>\/\?\s]+)/g,
    comments: {
        lineComment: "//",
        // blockComment: ["/*", "*/"]
    },
    brackets: [
        ["{", "}"],
        ["[", "]"],
        ["(", ")"],
    ],
    surroundingPairs: [
      { open: '{', close: '}' },
      { open: '[', close: ']' },
      { open: '(', close: ')' },
      //{ open: '<', close: '>' },
      //{ open: "'", close: "'" },
      { open: '"', close: '"' },
    ],
    autoClosingPairs: [
      { open: '{', close: '}' },
      { open: '[', close: ']' },
      { open: '(', close: ')' },
      //{ open: "'", close: "'", notIn: ['string', 'comment'] },
      { open: '"', close: '"', notIn: ['string', 'comment'] },
    ],
    folding: {
        markers: {
            start: new RegExp("^\\s*//\\s*#?region\\b"),
            end: new RegExp("^\\s*//\\s*#?endregion\\b")
        },
    },
};




const config = {
    allvalues: allvalues,
    lox_lang_config: lox_lang_config,
    makeTokensProvider: makeTokensProvider,
    ace_theme_monaco: ace_theme_monaco,
};
export default config;