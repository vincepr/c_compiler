#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "getFiles.h"
#include "binToStr.h"
#include "doUnitTest.h"




int main(){
    // 
    char* argv[3];
    argv[0] = "./binary.out";
    argv[1] = "test.lox";
    argv[2] = NULL;

    // our 'paths' defined (should probably argv[.] them out):
    char* bin = "./binary.out";     // path to the lox-interpreter-binary we test
    char* path = "./tests/files/";   // path pointing to our test-folder-structure


    // read all files (all files in test/files/...*.lox)
    int count_files = 0;
    char (*files)[30] = getFiles(path, &count_files); // we get our static array of all files:
    printf("found [%d] *.lox-files.\t starting unit tests now.\n", count_files);

    // Do a unit Test for each file:


    for (int i=0; i<count_files; i++){
        printf("testing file[%d]: %s\n",i, files[i]);
        bool didPass = doUnitTest(files[i], bin);
        if (didPass) {
        }
        

    }


    // // Runs the .binary.out est.lox and pipes it into string
    // char* argv[3];
    // argv[0] = "./binary.out";
    // argv[1] = "test.lox";
    // argv[2] = NULL;
    // char* out = runCmdToStr(argv, 0);
    // printf("%s", out);
    // free(out);



    exit(0);
}

