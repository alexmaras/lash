#ifndef LASH_H_
#define LASH_H_

#include "lashparser.h"

void emptyArray(char **array, int length);
int executeCommand(struct LashParser *parser);
void sighandler(int signum);
void runLash();
int runShellCommand(struct LashParser *parser, struct Command *command);
int runCommand(struct Command *command, int input);
int inArray(char *needle, char *haystack[], int length);

#endif
