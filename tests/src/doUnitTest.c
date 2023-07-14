#include <stdbool.h>
#include <stdlib.h>

#include <stdio.h>

#include "doUnitTest.h"
#include "binToStr.h"

bool doUnitTest(char* filepath, char* bin) {
    char* cmdToRun[3];
    cmdToRun[0] = bin;
    cmdToRun[1] = filepath;
    cmdToRun[2] = NULL;

    char* out = runCmdToStr(cmdToRun, 0);
    // TODO: read file-> 
    // TODO: string -> 
    // TODO: parse line by line and grep -> 
    // TODO: "//expect: "+++tillEndline+++ -> string array
    // TODO: substring search results -> if any not found -> false else true
    // TODO: if test failed we want real string vs expected string? ONLY LINE maybe?
    // TODO:    better to just print our string-array vs out-string
    printf("%s\n", out);
    free(out);
}