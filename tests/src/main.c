#include <stdio.h>
#include <stdlib.h>

#include "binToStr.h"
#include "getFiles.h"

int main(){
    getFiles();

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

