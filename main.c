#include <stdlib.h>

#include "lash.h"
#include "lashparser.h"

#define MAXCOMMANDS 100
#define MAXARGS 1000
#define MAXARGLENGTH 100
#define MAXPROMPT 40

int main(void){
	runLash(MAXCOMMANDS, MAXARGS, MAXARGLENGTH, MAXPROMPT);
    exit(0);
}
