#ifndef LASH_H_
#define LASH_H_

void emptyArray(char **array, int length);
void executeCommand(char **args, int argnum, int *pipeOrRedirect);
void sighandler(int signum);
void runLash();

#endif
