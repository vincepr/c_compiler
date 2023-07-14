#ifndef unit_getFiles_h
#define unit_getFiles_h

#define NUMBER_OF_STRING 200
#define MAX_STRING_SIZE 30

// char (*getFiles(void))[MAX_STRING_SIZE];     // minimal fixed-size static array function
char (*getFiles(char* path, int* count))[MAX_STRING_SIZE];

#endif