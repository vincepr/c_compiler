# As long as project compiles in a under a few seconds no need to get fancy and just build everything

## list all cfiles (.c) that our project includes (doing this manually for now)
CCFILES=main.c chunk.c memory.c debug.c value.c vm.c compiler.c scanner.c object.c table.c

## name of our executable we build to run
BINARY=binary.out


# builds out the binary
build:
	gcc -o $(BINARY) $(CCFILES)

# first build then run the binary
run: build
	./binary.out
#	./binary.out test.lox

# to remove all artifacts/binary
clean:
	rm -rf $(BINARY) *.o