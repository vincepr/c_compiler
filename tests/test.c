#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char **argv) {
    char* cmd = "./binary.out ./tests/unit.lox";       // 2>&1 redirects the errors->stdout
    printf("Running '%s'\n", cmd);

    FILE *fp = popen(cmd, "r");
    if (!fp){
        perror("ERROR - opening file - popen() failed:");
        exit(1);
    }
    printf("fp open file: '%s'\n'", cmd);

    char inLine[1024];
    while (fgets(inLine, sizeof(inLine), fp) != NULL){
        printf("Received: '%s'", inLine);     // a line from std-out
    }

    printf("feof=%d ferror=%d: %s\n", feof(fp), ferror(fp), strerror(errno));
    pclose(fp);

    return 0;
}