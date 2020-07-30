#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include "functions.h"

int main(int argc, char* argv[]){

    // id2 = argv[1] || mirrorDir = argv[2]

    char path[PATH_MAX];
    sprintf(path, "%s/%s/", argv[2], argv[1]);

    if(removeDirectory(path) == 0)
        printf("%s deleted successfully.\n", path);

    exit(EXIT_SUCCESS);
}