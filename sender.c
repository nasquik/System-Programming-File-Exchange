#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <dirent.h>
#include <signal.h>
#include <inttypes.h> 
#include <sys/stat.h> 
#include <sys/types.h>
#include <sys/inotify.h>
#include "functions.h"

// global variables //

int fd;
pid_t receiver_pid;

// signal handlers //

void receiverTermination(int signum){
    printf("Hello!I was triggered by a signal sent from my receiver counterpart.\n");
    close(fd);
    kill(getppid(), SIGUSR1);
    exit(EXIT_FAILURE);
}

void parentTermination(int signum){
    printf("Hello! I was triggered by a signal sent from my parent process.\n");
    close(fd);
    kill(receiver_pid, SIGUSR1);
    exit(EXIT_FAILURE);
}

// function definitions //

int copyFile(char*, int, int);
int copyAllFiles(const char*, int, char*, int, int);

// main function //

int main(int argc, char** argv){

    int id1 = atoi(argv[1]);
    int id2 = atoi(argv[2]);
    char* commonDir = argv[3];
    char* inputDir = argv[4];
    char* mirrorDir = argv[5];
    int bufferSize = atoi(argv[6]);
    int logFileDesc = atoi(argv[7]);

    //printf("Client %d sender.\n", id1);

    // handle signals sent ny receiver //

    struct sigaction receiverTerminator = {};
    
    receiverTerminator.sa_handler = receiverTermination;
    
    if(sigaction(SIGUSR1, &receiverTerminator, NULL) == -1){
     	perror("Error in sigaction");
        kill(getppid(), SIGUSR1);
	 	exit(EXIT_FAILURE);
	}

    // handle signals sent by parent //

    struct sigaction parentTerminator = {};
    
    parentTerminator.sa_handler = parentTermination;
    
    if(sigaction(SIGUSR2, &parentTerminator, NULL) == -1){
     	perror("Error in sigaction");
        kill(getppid(), SIGUSR1);
	 	exit(EXIT_FAILURE);
	}

    // create and/or open fifo //

    char myfifo[PATH_MAX];

    sprintf(myfifo, "./%s/%d_to_%d.fifo", commonDir, id1, id2);
    mkfifo(myfifo, 0666);
    unsigned short end = 0;
    unsigned int my_pid = getpid();

    fd = open(myfifo, O_RDONLY);

    // exchange PIDs with receiver counterpart //

    if(fd > 0){
        read(fd, &receiver_pid, 4);
        // printf("I am Sender %d. My pid is %d.\n", id1, my_pid);
        // printf("According to my pipe, Receiver %d's pid is %d.\n", id2, receiver_pid);
    }

    close(fd);

    fd = open(myfifo, O_WRONLY);

    if(fd > 0){

        write(fd, &my_pid, 4);

        //sleep(31); // only used for debug purposes

        // copy all files into FIFO //

        if(copyAllFiles(inputDir, fd, myfifo, bufferSize, logFileDesc) == 0){
            printf("All files sent successfully.\n");
            write(fd, &end, 2);
        }
        else{
            printf("Error. One or more files were not sent.\n");
            kill(getppid(), SIGUSR1);
            kill(receiver_pid, SIGUSR1);
            close(fd);
            exit(EXIT_FAILURE);
        }
    }
    else{
        printf("Error. FIFO did not open.\n");
        kill(getppid(), SIGUSR1);
        kill(receiver_pid, SIGUSR1);
        exit(EXIT_FAILURE);
    }

    kill(getppid(), SIGUSR2); // success!
    close(fd);

    exit(EXIT_SUCCESS);
}

int copyFile(char* name1, int outfile, int bufferSize){

    int infile;
    ssize_t nread;
    char buffer [bufferSize];

    if((infile = open(name1, O_RDONLY)) == -1)
        return -1;

    while((nread = read(infile , buffer, bufferSize)) > 0){
        if (write(outfile , buffer, nread) < nread){
            close(infile);
            return -3;
        }
    }

    close(infile);

    if(nread == -1){
        return -4;
    }
    else
        return 0;
}

int copyAllFiles(const char *name, int fd, char* fifo, int bufferSize, int logFileDesc){

    DIR *dir;
    struct dirent *entry;
    struct stat st;
    unsigned short nameSize;
    unsigned int size;
    int flag = 0;
    int counter = 0;
    FILE* logFile = fdopen(logFileDesc, "a");
     
    if (!(dir = opendir(name)))
        return -1;

    char currDir[PATH_MAX];

    while((entry = readdir(dir)) != NULL){
        counter = 0;
        if(entry->d_type == DT_DIR){
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            sprintf(currDir, "%s/%s", name, entry->d_name);
            flag = 1;
            if(copyAllFiles(currDir, fd, fifo, bufferSize, logFileDesc) < 0)
                return -1;
        }
        else{
            char fileName[PATH_MAX];
            char* shortenedPath;
            sprintf(fileName, "%s/%s", name, entry->d_name);
            shortenedPath = strchr(fileName, '/');
            shortenedPath = shortenedPath + 1;
            shortenedPath = strchr(shortenedPath, '/');
            shortenedPath = shortenedPath + 1;

            if(stat(fileName, &st) == 0)
                size = st.st_size;

            flag = 1;
            nameSize = strlen(shortenedPath) + 1;
            write(fd, &nameSize, 2);
            counter += 2;

            write(fd, shortenedPath, nameSize);
            counter += nameSize;

            write(fd, &size, 4);
            counter += 4;

            if((copyFile(fileName, fd, bufferSize)) == 0){
                
                counter += size;
                char string[2500];
                sprintf(string, "s f %s %d", shortenedPath, counter);
                writeToFile(logFile, logFileDesc, string);
            }
            else
                return -1;
        }
    }

    if(flag == 0){

            counter = 0;
            char dirName[PATH_MAX];
            strcpy(dirName, name);
            strcat(dirName, "/");
            char* shortenedPath;
            shortenedPath = strchr(dirName, '/');
            shortenedPath = shortenedPath + 1;
            shortenedPath = strchr(shortenedPath, '/');
            shortenedPath = shortenedPath + 1;
            nameSize = strlen(shortenedPath) + 1;
            write(fd, &nameSize, 2);
            counter += 2;
            write(fd, shortenedPath, nameSize);
            counter += nameSize;
            char string[2500];
            sprintf(string, "s d %s %d", shortenedPath, counter);
            writeToFile(logFile, logFileDesc, string);
    }

    closedir(dir);
    return 0;
}