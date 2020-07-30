OBJS = main.o clientInfo.o functions.o processCreator.o mirrorDestroyer.o sender.o receiver.o
SOURCE = main.c clientInfo.c functions.c processCreator.c mirrorDestroyer.c sender.c receiver.c
HEADER = clientInfo.h functions.h

CC = gcc
FLAGS = -g -c

all: mirror_client sender receiver processCreator mirrorDestroyer

mirror_client : main.o clientInfo.o functions.o
	$(CC) -g main.o clientInfo.o functions.o -o mirror_client

sender : sender.o functions.o
	$(CC) -g sender.o functions.o -o sender

receiver : receiver.o functions.o
	$(CC) -g receiver.o functions.o -o receiver

processCreator : processCreator.o functions.o
	$(CC) -g processCreator.o functions.o -o processCreator

mirrorDestroyer : mirrorDestroyer.o functions.o
	$(CC) -g mirrorDestroyer.o functions.o -o mirrorDestroyer

main.o : main.c
	$(CC) $(FLAGS) main.c

clientInfo.o : clientInfo.c
	$(CC) $(FLAGS) clientInfo.c

functions.o : functions.c
	$(CC) $(FLAGS) functions.c

processCreator.o : processCreator.c
	$(CC) $(FLAGS) processCreator.c

mirrorDestroyer.o : mirrorDestroyer.c
	$(CC) $(FLAGS) mirrorDestroyer.c

sender.o : sender.c
	$(CC) $(FLAGS) sender.c

receiver.o : receiver.c
	$(CC) $(FLAGS) receiver.c

clean :
	rm -f $(OBJS) mirror_client sender receiver processCreator mirrorDestroyer