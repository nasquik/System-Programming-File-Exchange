#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/file.h>
#include <dirent.h>
#include <signal.h>
#include <inttypes.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h>
#include <sys/inotify.h>
#include "functions.h"

// global variables //

int terminate = 0;
int fd;
pid_t sender_pid;

// signal handlers //

void alarmTermination(int signum){
    printf("Hello! I was triggered by an alarm. Setting terminate flag to 1.\n");
    terminate = 1;
}

void senderTermination(int signum){
    printf("Hello!I was triggered by a signal sent from my sender counterpart.\n");
    close(fd);
    kill(getppid(), SIGUSR1);
    exit(EXIT_FAILURE);
}

void parentTermination(int signum){
    printf("Hello! I was triggered by a signal sent from my parent process.\n");
    close(fd);
    kill(sender_pid, SIGUSR1);
    exit(EXIT_FAILURE);
}

// function definition //

int copyFile(int, char*, int, unsigned int);

// main function //

int main(int argc, char** argv){

    int id1 = atoi(argv[1]);
    int id2 = atoi(argv[2]);
    char* commonDir = argv[3];
    char* inputDir = argv[4];
    char* mirrorDir = argv[5];
    int bufferSize = atoi(argv[6]);
    int logFileDesc = atoi(argv[7]);

    // printf("Client %d receiver.\n", id1);

    // handle signals sent by alarm function
    
    struct sigaction alarmTerminator = {};
    
    alarmTerminator.sa_handler = alarmTermination;
    alarmTerminator.sa_flags   = SA_NODEFER;
    
    if(sigaction(SIGALRM, &alarmTerminator, NULL) == -1){
     	perror("Error in sigaction");
	 	exit(EXIT_FAILURE);
	}

    // handle signals sent by sender

    struct sigaction senderTerminator = {};
    
    senderTerminator.sa_handler = senderTermination;
    
    if(sigaction(SIGUSR1, &senderTerminator, NULL) == -1){
     	perror("Error in sigaction");
	 	exit(EXIT_FAILURE);
	}

    // handle signals sent by parent

    struct sigaction parentTerminator = {};
    
    parentTerminator.sa_handler = parentTermination;
    
    if(sigaction(SIGUSR2, &parentTerminator, NULL) == -1){
     	perror("Error in sigaction");
        kill(getppid(), SIGUSR1);
	 	exit(EXIT_FAILURE);
	}

    // create and/or open FIFO //

    char myfifo[PATH_MAX];

    sprintf(myfifo, "./%s/%d_to_%d.fifo", commonDir, id2, id1);
    mkfifo(myfifo, 0666);

    unsigned int my_pid = getpid();
    FILE* logFile = fdopen(logFileDesc, "a");

    fd = open(myfifo, O_WRONLY);

    // exchange PIDs with sender counterpart //

    if(fd > 0){
        write(fd, &my_pid, 4);
    }

    close(fd);

    fd = open(myfifo, O_RDONLY);

    if(fd > 0){

        read(fd, &sender_pid, 4);

        // printf("I am Receiver %d. My pid is %d.\n", id1, my_pid);
        // printf("According to my pipe, Sender %d's pid is %d.\n", id2, sender_pid);
        unsigned short nameSize;
        unsigned int size;
        char path[PATH_MAX];
        char* token;
        int counter;

        sprintf(path, "%s/%d/", mirrorDir, id2);
        mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

        // copy all files from FIFO //

        while(1){

            counter = 0;

            sprintf(path, "%s/%d/", mirrorDir, id2);
            alarm(30);
            read(fd, &nameSize, 2);
            alarm(0);
            if(terminate){
                unlink(myfifo);
                close(fd);
                kill(getppid(), SIGUSR1);
                kill(sender_pid, SIGUSR1);
                return -1;
            }

            counter += 2;

            if(nameSize == 0){
                printf("All files received successfully.\n");
                break;
            }

            char fileName[nameSize];

            alarm(30);
            read(fd, fileName, nameSize);
            alarm(0);
            if(terminate){
                unlink(myfifo);
                close(fd);
                kill(getppid(), SIGUSR1);
                kill(sender_pid, SIGUSR1);
                return -1;
            }
            counter += nameSize;

            if(strlen(fileName) + 1 != nameSize){
                printf("Error! The size of the name sent doesn't match the one previously stated.\n");
                return -1;
            }

            char name[nameSize];
            strcpy(name, fileName);

            strcat(fileName, ".");  // used only to differentiate between (empty) directories and files 
                                    //so that the right action is taken(starring Liam Neeson)

            token = strtok(fileName, "/");
            strcat(path, token);
            strcat(path, "/");
            mkdir(path,  S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            token = strtok(NULL, "/");
            int flag = 0;

            while(token != NULL){

                if(strcmp(token, ".") != 0 && token[strlen(token) - 1] == '.'){
                    flag = 1;

                    read(fd, &size, 4);
                    counter += 4;

                    token[strlen(token) - 1] = '\0';
                    strcat(path, token);

                    if(copyFile(fd, path, bufferSize, size) == 0){
                        counter += size;
                        char string[2500];
                        sprintf(string, "r f %s %d", name, counter);
                        writeToFile(logFile, logFileDesc, string);
                    }
                    else{
                        unlink(myfifo);
                        close(fd);
                        kill(getppid(), SIGUSR1);
                        kill(sender_pid, SIGUSR1);
                        return -1;
                    }
                }
                else{
                    strcat(path, token);
                    strcat(path, "/");
                    mkdir(path,  S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
                }
                token = strtok(NULL, "/");
            }
            if(flag == 0){
                char string[2500];
                sprintf(string, "r d %s %d", name, counter);
                writeToFile(logFile, logFileDesc, string);
            }
        }
    }
    else{
        printf("Error. FIFO did not open.\n");
        kill(getppid(), SIGUSR1);
        kill(sender_pid, SIGUSR1);
        return -1;
    }

    close(fd);
    unlink(myfifo);
    kill(getppid(), SIGUSR2);   // success!
    exit(EXIT_SUCCESS); 
}

int copyFile(int infile, char* name2, int bufferSize, unsigned int size){

    int outfile;
    ssize_t nread;
    char buffer[bufferSize];
    int counter = 0;

    if ((outfile = open(name2, O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1){
        return -1;
    }

    int times = size / bufferSize;
    int left = size % bufferSize;

    for(int i = 0; i < times; i++){
        alarm(30);
        nread = read(infile, buffer, bufferSize);
        alarm(0);
        if(terminate){
            close(outfile);
            return -1;
        }

        counter += nread;

        if(write(outfile, buffer, nread) < nread){
            close (outfile);
            return -1;
        }
    }

    alarm(30);
    nread = read(infile, buffer, left);
    alarm(0);
    if(terminate){
        close(outfile);
        return -1;
    }
    counter += nread;
    if(write(outfile, buffer, nread) < nread){
        close (outfile);
        return -1;
    }

    if(counter != size){
        close(outfile);
        return -1;
    }

    close (outfile);

    if(nread == -1)
        return -4;
    else
        return 0;
}