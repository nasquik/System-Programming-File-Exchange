#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/file.h>
#include <sys/inotify.h>
#include "functions.h"

const char* target_type(struct inotify_event* event){
	if( event->len == 0 )
		return "";
	else
		return event->mask & IN_ISDIR ? "directory" : "file";
}

const char* target_name(struct inotify_event* event){
	return event->len > 0 ? event->name : NULL;
}

const char* event_name(struct inotify_event* event){
	if (event->mask & IN_CREATE)
		return "created";
	else if (event->mask & IN_DELETE)
		return "deleted";
	else
		return "modified due to unknown causes";
}

int writeToFile(FILE* file, int fileDesc, char* string){

	if(flock(fileDesc,LOCK_EX) < 0)
        return -1;

	fprintf(file, "%s\n", string);

	if(fsync(fileDesc) < 0)
     	return -1;

	if(flock(fileDesc,LOCK_UN) < 0)
        return -1;

	return 0;
}

int removeDirectory(char* name){

    DIR *dir;
    struct dirent *entry;
    struct stat st;
    int flag = 0;
     
    if (!(dir = opendir(name)))
        return -1;

    char currDir[PATH_MAX];

    while((entry = readdir(dir)) != NULL){
        if(entry->d_type == DT_DIR){
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            sprintf(currDir, "%s/%s", name, entry->d_name);
            removeDirectory(currDir);
        }
        else{
            char fileName[PATH_MAX];
            char* shortenedPath;
            sprintf(fileName, "%s/%s", name, entry->d_name);
            unlink(fileName);
        }
    }
    closedir(dir);
    rmdir(name);
    return 0;
}