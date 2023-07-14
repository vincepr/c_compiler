#include <dirent.h>
#include <stdio.h>
#include <string.h>

#define NUMBER_OF_STRING 200
#define MAX_STRING_SIZE 30
int CURRENT_STR_ARRAY_SIZE = 0;

// gets all .lox files we want to parse  
void getFiles() {
    DIR *d;
    struct dirent *dir;
    d = opendir("./tests/files");
    char arr[NUMBER_OF_STRING][MAX_STRING_SIZE];

    if (d){
        while ((dir = readdir(d)) != NULL){
            char * pch = strstr(dir->d_name, ".lox");
            if (pch != NULL) {
                printf("found files: %s\n", dir->d_name);
                *arr[CURRENT_STR_ARRAY_SIZE] = *dir->d_name;
                CURRENT_STR_ARRAY_SIZE += 1;
            }
        }
        closedir(d);
        printf("arr[1]: %s\n", arr[1] );
    }
    return;
}