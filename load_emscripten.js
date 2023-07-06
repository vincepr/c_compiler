var statusElement = document.getElementById('status');
var progressElement = document.getElementById('progress');
var spinnerElement = document.getElementById('spinner');

var Module = {
  preRun: [
    function() {
      function stdin() {
        if (i < res.length) {
          var code = input.charCodeAt(i);
          ++i;
          return code;
        } else {
          return null;
        }
      }

      var stdoutBuffer = "";
      function stdout(code) {
        if (code === "\n".charCodeAt(0) && stdoutBuffer !== "") {
          console.log(stdoutBuffer);
          stdoutBufer = "";
        } else {
          stdoutBuffer += String.fromCharCode(code);
        }
      }

      var stderrBuffer = "";
      function stderr(code) {
        if (code === "\n".charCodeAt(0) && stderrBuffer !== "") {
          console.log("BUFFERED ERROR:! " + stderrBuffer);
          stderrBuffer = "";
        } else {
          stderrBuffer += String.fromCharCode(code);
        }
      }
      FS.init(stdin, stdout, stderr);
    }
  ],
  
  postRun: [],

  print: (function() {
    var element = document.getElementById('output');
    if (element) element.value = ''; // clear browser cache
    return function(text) {
      if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');
      // These replacements are necessary if you render to raw HTML
      //text = text.replace(/&/g, "&amp;");
      //text = text.replace(/</g, "&lt;");
      //text = text.replace(/>/g, "&gt;");
      //text = text.replace('\n', '<br>', 'g');
      console.log(text);
      if (element) {
        element.value += text + "\n";
        element.scrollTop = element.scrollHeight; // focus on bottom
      }
    };
  })(),

  setStatus: (text) => {
    if (!Module.setStatus.last) Module.setStatus.last = { time: Date.now(), text: '' };
    if (text === Module.setStatus.last.text) return;
    var m = text.match(/([^(]+)\((\d+(\.\d+)?)\/(\d+)\)/);
    var now = Date.now();
    if (m && now - Module.setStatus.last.time < 30) return; // if this is a progress update, skip it if too soon
    Module.setStatus.last.time = now;
    Module.setStatus.last.text = text;
    if (m) {
      text = m[1];
      progressElement.value = parseInt(m[2])*100;
      progressElement.max = parseInt(m[4])*100;
      progressElement.hidden = false;
      spinnerElement.hidden = false;
    } else {
      progressElement.value = null;
      progressElement.max = null;
      progressElement.hidden = true;
      if (!text) spinnerElement.hidden = true;
    }
    statusElement.innerHTML = text;
  },

  totalDependencies: 0,

  monitorRunDependencies: (left) => {
    this.totalDependencies = Math.max(this.totalDependencies, left);
    Module.setStatus(left ? 'Preparing... (' + (this.totalDependencies-left) + '/' + this.totalDependencies + ')' : 'All downloads complete.');
  }
};



Module.setStatus('Downloading...');
window.onerror = () => {
  Module.setStatus('Exception thrown, see JavaScript console');
  spinnerElement.style.display = 'none';
  Module.setStatus = (text) => {
    if (text) console.error('[post-exception status] ' + text);
  };
};