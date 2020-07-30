#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h> 
#include <sys/file.h>
#include <sys/inotify.h>
#include <sys/wait.h>
#include "clientInfo.h"
#include "functions.h"

#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + 16))

int terminate = 0;

void terminationHandler(int signum){
    terminate = 1;
}

int main(int argc, char* argv[]){

    //////// ARGUMENTS ////////

    struct clientInfo* clInfo = NULL;

    if(argc < 13 || argc > 13){
        printf("Incorrect Input!\n");
        return -1;
    }
    else{
        clInfo = clientInfoInit(&argc, argv);
        if(clInfo == NULL){
            return -1;
        }
    }

    //////// CHECK DIRECTORIES ////////

    if(createDirectories(clInfo) == -1)
        return -1;

    //////// CREATE FILE <ID>.ID IN COMMON FOLDER AND LOG FILE ////////

    FILE *file;
    char fileName[51];
    
    sprintf(fileName, "%s/%s.id", clInfo->commonDir, clInfo->id);

    file = fopen(fileName, "w");
    
    if (file == NULL)
    {
        perror("Error!");
        return -1;
    }

    fprintf(file, "%d", getpid());

    fclose(file);

    FILE* logFile;
    int logFileDesc;

    logFile = fopen(clInfo->logFile, "w");
    logFileDesc = fileno(logFile);
    char fileDesc[12];
    sprintf(fileDesc, "%d", logFileDesc);
    char string[51];

    sprintf(string, "Client %s created", clInfo->id);

    writeToFile(logFile, logFileDesc, string);

    //////// SET UP SIGNAL HANDLERS ////////

    struct sigaction terminator = {};
    
    terminator.sa_handler = terminationHandler;
    terminator.sa_flags   = SA_NODEFER;
    
    if(sigaction(SIGINT, &terminator, NULL) == -1){
     	perror("Error in sigaction");
	 	exit(EXIT_FAILURE);
	}
    
    if(sigaction(SIGQUIT, &terminator, NULL) == -1){
     	perror("Error in sigaction");
	 	exit(EXIT_FAILURE);
	}

    //////// SYNC CLIENT WITH CLIENTS IN COMMON FOLDER ////////

    char name[21];
    char* id2;
    char* extension;
    pid_t pid, wpid;

    DIR *dir;
    struct dirent* dirent;
    dir = opendir(clInfo->commonDir);
   
    if(dir){
        while ((dirent = readdir(dir)) != NULL){
            
            if(dirent->d_type == DT_DIR)
                continue;

            strcpy(name, dirent->d_name);
            id2 = strtok(name, ".");
            extension = strtok(NULL, "\0");

            if(strcmp(id2, clInfo->id) == 0 || strcmp(extension, "id") != 0)
                continue;

            pid = fork();

            if(pid == 0)
                execl("./processCreator", "processCreator", clInfo->id, id2, clInfo->commonDir, clInfo->inputDir, clInfo->mirrorDir, clInfo->bufferSize, fileDesc, NULL);                
            else if(pid < 0)
                perror("Fork failed. Could not create Child Creating Process");
        }
        closedir(dir);
    }

    //////// INOTIFY SETUP ////////

	int length, read_ptr, read_offset;
    int wd, fd;
	char buffer[EVENT_BUF_LEN];

    int status = 0;

	fd = inotify_init();
	if (fd < 0)
		perror("Error with INOTIFY initialization.");

    wd = inotify_add_watch(fd, clInfo->commonDir, IN_CREATE | IN_DELETE);

    if(wd == -1)
        printf("Failed to add watch on %s.\n", clInfo->commonDir);
    else
        printf("Watching %s.\n", clInfo->commonDir);

    //////// WATCHING COMMON DIRECTORY ////////

	read_offset = 0;

    while(1){

		length = read(fd, buffer + read_offset, sizeof(buffer) - read_offset);
        if(terminate)
            break;

		if (length < 0)
            perror("Error reading.");
		length += read_offset;
		read_ptr = 0;
		
		while(read_ptr + EVENT_SIZE <= length){

			struct inotify_event* event = (struct inotify_event*) &buffer[read_ptr];
			if(read_ptr + EVENT_SIZE + event->len > length) 
				break;

            if(event->mask == IN_CREATE && strcmp(target_type(event), "file") == 0){

                strcpy(name, target_name(event));
			    printf("File %s was created.\n", name);     // if file created belongs to another client
                                                            // create child processes
                id2 = strtok(name, ".");
                extension = strtok(NULL, "\0");

                if(strcmp(extension, "id") != 0 || strcmp(id2, clInfo->id) == 0){
                    read_ptr += EVENT_SIZE + event->len;
                    continue;
                }

                pid = fork();
                if(pid == 0)
                    execl("./processCreator", "processCreator", clInfo->id, id2, clInfo->commonDir, clInfo->inputDir, clInfo->mirrorDir, clInfo->bufferSize, fileDesc, NULL);                
                else if(pid < 0)
                    perror("Fork failed. Could not create Child Creating Process.");

            }
            else if(event->mask == IN_DELETE && strcmp(target_type(event), "file") == 0){
			    strcpy(name, target_name(event));
			    printf("File %s was deleted.\n", name);     // if file deleted belongs to a client
                                                            // remove according file from mirror dir
                id2 = strtok(name, ".");
                extension = strtok(NULL, "\0");

                if(strcmp(extension, "id") != 0 || strcmp(id2, clInfo->id) == 0){
                    read_ptr += EVENT_SIZE + event->len;
                    continue;
                }

                pid = fork();
                if(pid == 0)
                    execl("./mirrorDestroyer", "mirrorDestroyer", id2, clInfo->mirrorDir, NULL);                
                else if(pid < 0)
                    perror("Fork failed. Could not create Mirror Destroying Process.");
            }
            else{
                printf("Unknown type of change was made.\n");
            }
			read_ptr += EVENT_SIZE + event->len;
		}
		if(read_ptr < length) {
			memcpy(buffer, buffer + read_ptr, length - read_ptr);
			read_offset = length - read_ptr;
		} else
			read_offset = 0;
		
	}
    
    wait(NULL);
    printf("\nClient is exiting.\n");
    removeDirectory(clInfo->mirrorDir);
    
    char idFile[PATH_MAX];
    sprintf(idFile, "%s/%s.id", clInfo->commonDir, clInfo->id);
    unlink(idFile);

    char exitString[51];
    sprintf(exitString, "Client %s exited", clInfo->id);
    writeToFile(logFile, logFileDesc, exitString);
    
    if(wd && fd)
        inotify_rm_watch(fd, wd);
    
    if(fd)
        close(fd);

    if(logFile)
        fclose(logFile);
    
    if(clInfo)
        free(clInfo);
    
    exit(EXIT_SUCCESS);
}