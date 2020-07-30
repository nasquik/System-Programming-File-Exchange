#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include "clientInfo.h"

clientInfo* clientInfoInit(int* argc, char** argv){

    clientInfo* clInfo = malloc(sizeof(clientInfo));

    for(int i = 1; i <= *argc-1; i+=2){

        if(strcmp(argv[i], "-n") == 0)
            clInfo->id = argv[i+1];
        else if(strcmp(argv[i], "-c") == 0)
            clInfo->commonDir = argv[i+1];
        else if(strcmp(argv[i], "-i") == 0)
            clInfo->inputDir = argv[i+1];
        else if(strcmp(argv[i],"-b") == 0)
            clInfo->bufferSize = argv[i+1];
        else if(strcmp(argv[i], "-m") == 0)
            clInfo->mirrorDir = argv[i+1];
        else if(strcmp(argv[i], "-l") == 0)
            clInfo->logFile = argv[i+1];
        else{
            printf("Incorrect Input!\n");
            free(clInfo);
            return NULL;
        }
    }

    return clInfo;
}

int createDirectories(clientInfo* clientInfo){
    
    DIR* dir[3];
    
    dir[0] = opendir(clientInfo->inputDir);

    if (dir[0]){
        closedir(dir[0]);
    }
    else if (ENOENT == errno){
        perror("Error!");
        return -1;
    }
    else{
        perror("Error!");
        return -1;
    }

    dir[1] = opendir(clientInfo->mirrorDir);

    if (dir[1]){
        printf("Error! Mirror Directory already exists!\n");
        closedir(dir[1]);
        return -1;
    }
    else{
        if(mkdir(clientInfo->mirrorDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0){
            printf("Mirror Directory Created!\n");
        }
    }

    dir[2] = opendir(clientInfo->commonDir);

    if (dir[2]){
        closedir(dir[2]);
    }
    else{
        if(mkdir(clientInfo->commonDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0){
            printf("Common Directory Created!\n");
        }
    }

    return 0;
}