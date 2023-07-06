# As long as project compiles in a under a few seconds no need to get fancy and just build everything

## path to the src folder
CCPATH=src/

## list all cfiles (.c) that our project includes (doing this manually for now)
CCFILES= $(CCPATH)main.c \
$(CCPATH)chunk.c \
$(CCPATH)memory.c \
$(CCPATH)debug.c \
$(CCPATH)value.c \
$(CCPATH)vm.c \
$(CCPATH)compiler.c \
$(CCPATH)scanner.c \
$(CCPATH)object.c \
$(CCPATH)table.c

## list all cfiles included in our wasm-build:
WEBFILES= srcweb/main-web.c \
$(CCPATH)chunk.c \
$(CCPATH)memory.c \
$(CCPATH)debug.c \
$(CCPATH)value.c \
$(CCPATH)vm.c \
$(CCPATH)compiler.c \
$(CCPATH)scanner.c \
$(CCPATH)object.c \
$(CCPATH)table.c

## name of our executable we build to run
BINARY=binary.out


# builds out the binary
build:
	gcc -o $(BINARY) $(CCFILES)

# first build then run the binary
run: build
	./binary.out
#	./binary.out test.lox

test: build
	./binary.out test.lox

# build the wasm-build:
web: 
	emcc  $(WEBFILES) -o build_wasm/index.html --shell-file srcweb/shell_minimal.html -s NO_EXIT_RUNTIME=1 -s "EXPORTED_RUNTIME_METHODS=['ccall']"
	cp srcweb/load_emscripten.js build_wasm/load_emscripten.js
	cp srcweb/load_monaco.js build_wasm/load_monaco.js
	cp srcweb/styles.css build_wasm/styles.css
#	emcc $(WEBFILES) -o build_wasm/compiler.html -sEXPORTED_FUNCTIONS=_compileSourceCode -sEXPORTED_RUNTIME_METHODS=ccall,cwrap

# to remove all artifacts/binary
clean:
	rm -rf $(BINARY) *.o