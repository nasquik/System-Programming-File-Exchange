#ifndef FUNCTIONS_H
#define FUNCTIONS_H

const char* target_type(struct inotify_event*);
const char* target_name(struct inotify_event*);
int writeToFile(FILE*, int, char*);
int removeDirectory(char*);

#endif