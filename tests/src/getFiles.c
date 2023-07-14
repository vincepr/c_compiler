#include <dirent.h>
#include <stdio.h>
#include <string.h>

#include "getFiles.h"







// gets all .lox files we want to parse  
// - we use a static String array -> no need to free this!
// - count keeps track how many files we found
char (*getFiles(char* path, int* count))[MAX_STRING_SIZE] {
    DIR *d;
    struct dirent *dir;
    d = opendir(path);
    static char arr[NUMBER_OF_STRING][MAX_STRING_SIZE];
    if (d){
        while ((dir = readdir(d)) != NULL){
            char * pch = strstr(dir->d_name, ".lox");
            if (pch != NULL) {
                char copy_path[200];
                strcpy(copy_path,  path);
                strcat(copy_path, dir->d_name); // "/path/tests/files/" + "testingPrint.lox"
                strcpy(arr[*count], copy_path); // cant assing to string array directly have to copy into it
                *count += 1;
            }
        }
        closedir(d);
    }
    return arr;
}