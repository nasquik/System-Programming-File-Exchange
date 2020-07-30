#ifndef CLIENTINFO_H
#define CLIENTINFO_H

typedef struct clientInfo{
    char* commonDir;
    char* inputDir;
    char* mirrorDir;
    char* id;
    char* bufferSize;
    char* logFile;
}clientInfo;

clientInfo* clientInfoInit(int*, char**);
int createDirectories(clientInfo*);

#endif