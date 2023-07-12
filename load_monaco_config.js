
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

// // you can comment out whole lines with double slash
// for (var i=0; i<10; i=i+1){
//   print "for-loop:";
//   while (i<5) {
//     print i;
//     i = i+1;
//   }
//   print i;
// }

// lox uses dynamically typed variables:
var someValue = true;
someValue = 1234;
someValue = someValue + 5000 * (-55 /3);
print -1 * someValue;   // prints 90432.7

// and the usal comparisons and equality operators work:
var isTrue = 1 != 2;
if (!isTrue) print 99 > 1;
else {
  print "this is the happy path!";
}


// this should error out to show the stack-trace:
fun a() { b(); }
fun b() { c(); }
fun c() {
  c("too", "many"); // this call has too many arguments
}
a();    // this will produce some stack-trance, with the above error

// since this is a runttime error the code above this will execute.
// for a runtime error try removing a ';' or forget to close a bracket.
`,




    arrays_lox: `// I added Arrays (more a list like dynamic array) ontop of the default Lox-Language:


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
        ],

        symbols: /[=><!:+\-*\/,]+/,

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