#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include "functions.h"

// global variables //

int failure = 0;
int successCount = 0;

// signal handlers //

void successSignalHandler(int signum){
    printf("Hello! Parent process here. One of my children has succeeded.\n");
    successCount++;
}

void failureSignalHandler(int signum){               // if sender or receiver fails
    printf("Hello! Parent process here. One of my children has failed.\n");
    failure = 1;
}

// main function //

int main(int argc, char* argv[]){

    // id1 = argv[1] || id2 = argv[2] || commonDir = argv[3] || inputDir = argv[4] || 
    // mirrorDir = argv[5] || bufferSize = argv[6] || logFileDesc = argv[7];

    pid_t sender_pid, receiver_pid;
    int totalAttempts = 0;

    // handle success signals //

    struct sigaction successSignal = {};
    
    successSignal.sa_handler = successSignalHandler;
    
    if(sigaction(SIGUSR2, &successSignal, NULL) == -1){
     	perror("Error in sigaction.");
	 	exit(EXIT_FAILURE);
	}

    // handle failure signals //

    struct sigaction failureSignal = {};
    
    failureSignal.sa_handler = failureSignalHandler;
    
    if(sigaction(SIGUSR1, &failureSignal, NULL) == -1){
     	perror("Error in sigaction.");
	 	exit(EXIT_FAILURE);
	}

    // first attempt to create child processes //

    totalAttempts++;

    printf("Parent process creates children for the first time.\n");

    // sender //

    sender_pid = fork();

    if(sender_pid == 0)
        execl("./sender", "sender", argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], NULL);                
    else if(sender_pid < 0)
        perror("Fork failed. Could not create Sender Child Process");

    // receiver //
    
    receiver_pid = fork();

    if(receiver_pid == 0)
        execl("./receiver", "receiver", argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], NULL);                
    else if(receiver_pid < 0)
        perror("Fork failed. Could not create Receiver Child Process");

    // error handling && further attempts //

    while(1){

        wait(NULL);

        if(successCount == 2 || totalAttempts >= 3){
            break;
        }

        if(failure){

            totalAttempts++;
            successCount = 0;
            failure = 0;
            
            kill(sender_pid, SIGUSR2);
            kill(receiver_pid, SIGUSR2);

            while( wait(NULL) > 0 ); // wait for killed children to exit

            char mirrorPath[101];

            sprintf(mirrorPath, "%s/%s/", argv[5], argv[2]);
            removeDirectory(mirrorPath);

            // re-attempt to create child processess

            // sender //

            printf("Parent process recreates children. This is attempt No %d\n", totalAttempts);

            sender_pid = fork();

            if(sender_pid == 0)
                execl("./sender", "sender", argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], NULL);                
            else if(sender_pid < 0)
                perror("Fork failed. Could not create Sender Child Process.");

            // receiver //
            
            receiver_pid = fork();

            if(receiver_pid == 0)
                execl("./receiver", "receiver", argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], NULL);                
            else if(receiver_pid < 0)
                perror("Fork failed. Could not create Receiver Child Process.");
        }
    }

    // exit whether successful or not //
    
    while( wait(NULL) > 0 );

    if(successCount == 2){
        printf("File Transfer Successful!\n");
        exit(EXIT_SUCCESS);
    }

    if(totalAttempts >= 3){
        printf("File Tranfer Unsuccessful. Too many attempts were made.\n");
        exit(EXIT_FAILURE);
    }
}