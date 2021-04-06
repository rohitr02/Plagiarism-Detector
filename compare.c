#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "compare.h"


#ifndef DEBUG
#define DEBUG 0
#endif

int main(int argc, char* argv[]){
    /* Input Validity */
    if(argc < 2) return EXIT_FAILURE;                   // No arguments entered

}