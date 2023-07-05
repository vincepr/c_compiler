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



// Register the new language
monaco.languages.register({ id: "lox" });

// Register a tokens provider for the language
monaco.languages.setMonarchTokensProvider("lox", 
	makeTokensProvider()
);


// String that stores our 'preview'-files
const allfiles = {
    "hello.lox": {
        name: "hello.lox",
        language: "lox",
        theme: "vs-dark",
        value: 'print("James Bond")',
    },

    "fib.lox": {
        name: "fib.lox",
        language: "lox",
        theme: "vs-dark",
        value: `fun fib(n) {
  if (n < 2) return n;
  return fib(n - 2) + fib(n - 1);
}

var start = clock();
print fib(31);
print clock() - start;`,
    },

    "clojure.lox": {
        name: "clojure.lox",
        language: "lox",
        theme: "vs-dark",
        value: "not implemented"
    },
}


// set the monaco-editor-window:
var activeFile = allfiles["fib.lox"];
var editor = monaco.editor.create(document.getElementById('container'), activeFile);


// Add the event listeners (onClick events)


function handleClick(filename) {
    // var x = editor.getValue();
    // console.log(x)
    editor.setValue(allfiles[filename].value);
    //editor.getModel().setValue('some value')
}

document.getElementById("btnhello").addEventListener("click", () => {
    handleClick("hello.lox");
});

document.getElementById("btnfib").addEventListener("click", () => {
    handleClick("fib.lox");
});

document.getElementById("btnclojure").addEventListener("click", () => {
    handleClick("clojure.lox");
});

document.getElementById("btncompile").addEventListener("click", () => {
    let currentText = editor.getValue();
    document.getElementById('output').value="";

    const result = Module.ccall(
        "runCompiler", // name of C function
        null, // return type
        ["string"], // argument types
        [currentText] // arguments
    );
});