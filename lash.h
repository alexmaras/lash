#ifndef LASH_H_
#define LASH_H_

void emptyArray(char **array, int length);
void executeCommand(char **args);
void sighandler(int signum);
void runLash();

#endif
