

## list all cfiles (.c) that our project includes
CCFILES=main.c chunk.c memory.c

## name of our executable we build to run
BINARY=binary.out


build:
	gcc -o $(BINARY) $(CCFILES)

run: build
	./binary.out

## manual command to remove all artifacts
clean:
	rm -rf $(BINARY) *.o

# https://www.youtube.com/watch?v=DtGrdB8wQ_8&list=RDLV_r7i5X0rXJk&index=2