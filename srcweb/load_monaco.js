
/* Custom Editor Theme */
monaco.editor.defineTheme('ace', {
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
        { token: 'comment', foreground: '86b300', fontStyle: 'italic' },
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
});

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

const lox_lang_config = {
    //wordPattern: /(-?\d*\.\d\w*)|([^\`\~\!\@\#\%\^\&\*\(\)\-\=\+\[\{\]\}\\\|\;\:\'\"\,\.\<\>\/\?\s]+)/g,
    comments: {
        lineComment: "//",
        //blockComment: ["/*", "*/"]
    },
    brackets: [
        ["{", "}"],
        ["[", "]"],
        ["(", ")"]
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
        }
    },
  };

// Register the new language
monaco.languages.register({ id: "lox" });
monaco.languages.setLanguageConfiguration('lox', lox_lang_config);


// Register a tokens provider for the language
monaco.languages.setMonarchTokensProvider("lox", 
	makeTokensProvider()
);


// String that stores our 'preview'-files
const allvalues = {
    class_lox:  `// The Basics about classes \n
class Pair {}
var pair = Pair();  // Classes get instanciated with ClassName()
pair.first = 1;
pair.second = 2;
print pair.first + pair.second; // prints 3.
`,


    fib_lox: `// calculate a fibonacci-nr and return the time it took in seconds:\n
fun fib(n) {
  if (n < 2) return n;
  return fib(n - 2) + fib(n - 1);
}

var start = clock();
print fib(31);
print "time spent running in seconds:";
print clock() - start;`,


    closure_lox:  `// when closures are implemented this should print out outer:\n
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

    error_lox: `// For loops - (;;) would be infinite loop \n
for (var i=0; i<10; i=i+1){
  print "for-loop:";
  while (i<5) {
    print i;
    i = i+1;
  }
  print i;
}

// this should error out to show the stack-trace:
fun a() { b(); }
fun b() { c(); }
fun c() {
  c("too", "many"); // this call has too many arguments
}
a();    // this will produce some stack-trance, with the above error
`,
}


// set the monaco-editor-window:
var activeFile = "closure_lox"
let config_monaco = {
    name: "closure.lox",
    language: "lox",
    theme: "ace",
    autoClosingBrackets: true,      // not working! TODO: test if language implementation has to define those TODO: didnt it work with premade theme?
    minimap: { enabled: false },
    automaticLayout: true,          // resizes every 100ms or so if window-size changes
    wordWrap: 'on',
    scrollBeyondLastLine: false,
    //"bracketPairColorization.enabled": true,
    value: allvalues[activeFile],
}
var editor = monaco.editor.create(document.getElementById('container'), config_monaco);


// Add the event listeners (onClick events)


function changeOpenFile(filename) {
    allvalues[activeFile] = editor.getValue();     // persist change to 'file'-tab
    activeFile = filename;                              // set new active tab
    editor.setValue(allvalues[filename]);          // open clicked tab to editor
    //editor.getModel().setValue('some value')
}

document.getElementById("btnclass").addEventListener("click", () => {
    changeOpenFile("class_lox");
});

document.getElementById("btnfib").addEventListener("click", () => {
    changeOpenFile("fib_lox");
});

document.getElementById("btnclosure").addEventListener("click", () => {
    changeOpenFile("closure_lox");
});

document.getElementById("btnerror").addEventListener("click", () => {
    changeOpenFile("error_lox");
});

document.getElementById("btncompile").addEventListener("click", () => {
    // read text-sourcecode:
    let currentText = editor.getValue();
    document.getElementById('output').value="";
    // read out flags:
    let isBytecode = document.getElementById('bytecode').checked ==true;  // savety against undfefined by ==true
    let isTrace = document.getElementById('trace').checked ==true;
    let isGC = document.getElementById('gc').checked ==true;

        
    //progressElement.hidden = false;
    spinnerElement.hidden = false;
    // we have to wait for the hidden to get updated before we call the compile function
    // 34ms > 1frame on 30fps. thats as low as i'm willing to support
    const myTimeout = setTimeout(()=>compileCall({currentText:currentText, isBytecode:isBytecode, isTrace:isTrace, isGC:isGC}), 34);
});

function compileCall({currentText:currentText, isBytecode:isBytecode, isTrace:isTrace, isGC:isGC}) {
    const result = Module.ccall(
        "runCompiler", // name of C function
        "int", // return type
        ["string", "bool", "bool", "bool"], // argument types
        [currentText, isBytecode, isTrace, isGC] // arguments
    )
    // remove the spinner
    //progressElement.hidden = true;
    spinnerElement.hidden = true;
    //console.log("Return = "+ result);
    //return result;        // were not checking if we error'ed anyway
}