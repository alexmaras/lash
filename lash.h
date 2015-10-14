#ifndef LASH_H_
#define LASH_H_

#include <stdbool.h>
#include "lashparser.h"

void emptyArray(char **array, int length);
bool executeCommand(struct LashParser *parser);
void sighandler(int signum);
void runLash();

#endif
