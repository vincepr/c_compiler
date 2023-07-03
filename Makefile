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

# to remove all artifacts/binary
clean:
	rm -rf $(BINARY) *.o