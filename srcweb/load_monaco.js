import config from "./load_monaco_config.js";

// Register the new theme and language to monaco
monaco.editor.defineTheme('ace', config.ace_theme_monaco);
monaco.languages.register({ id: "lox" });
monaco.languages.setLanguageConfiguration('lox', config.lox_lang_config);


// Register a tokens provider for the language
monaco.languages.setMonarchTokensProvider("lox", 
	config.makeTokensProvider()
);

// set the monaco-editor-window:
var activeFile = "closure_lox"
let editor_settings = {
    name: "closure.lox",
    language: "lox",
    theme: "ace",
    autoClosingBrackets: true,      // not working! TODO: test if language implementation has to define those TODO: didnt it work with premade theme?
    minimap: { enabled: false },
    automaticLayout: true,          // resizes every 100ms or so if window-size changes
    wordWrap: 'on',
    scrollBeyondLastLine: false,
    "bracketPairColorization.enabled": false,
    value: config.allvalues[activeFile],
}
var editor = monaco.editor.create(document.getElementById('container'), editor_settings);


// Add the event listeners (onClick events)


function changeOpenFile(filename) {
    //
    document.getElementById(activeFile).classList.remove("fileopen");
    document.getElementById(filename).classList.add("fileopen");
    //
    config.allvalues[activeFile] = editor.getValue();     // persist change to 'file'-tab
    activeFile = filename;                              // set new active tab
    editor.setValue(config.allvalues[filename]);          // open clicked tab to editor
    //editor.getModel().setValue('some value')
}

document.getElementById("class_lox").addEventListener("click", () => {
    changeOpenFile("class_lox");
});
document.getElementById("fib_lox").addEventListener("click", () => {
    changeOpenFile("fib_lox");
});
document.getElementById("closure_lox").addEventListener("click", () => {
    changeOpenFile("closure_lox");
});
document.getElementById("error_lox").addEventListener("click", () => {
    changeOpenFile("error_lox");
});
document.getElementById("arrays_lox").addEventListener("click", () => {
    changeOpenFile("arrays_lox");
});
document.getElementById("custom_lox").addEventListener("click", () => {
    changeOpenFile("custom_lox");
});
document.getElementById("darkmode").addEventListener("click", ()=>{
    let isDarkmode = document.getElementById('darkmode').checked ==true;
    toggleDarkmodeCss(isDarkmode);
    if (isDarkmode) {
        editor._themeService.setTheme("vs-dark");
    } else {
        editor._themeService.setTheme("ace");
    }
});

// everything primed with class 'dark1' gets class 'dark2Active' toggled etc.
function toggleDarkmodeCss(isDark) {
    ["dark1", "dark2", "dark3", "dark4"].forEach((dm)=>{
        var objs = document.querySelectorAll("." + dm);
        if (isDark) objs.forEach((el) => el.classList.add(dm + "Active"));
        else objs.forEach((el) => el.classList.remove(dm + "Active"));
    });
}

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