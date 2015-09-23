#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#define MAXCOMMANDS 100
#define MAXARGS 1000
#define MAXARGLENGTH 100

char prompt[80];

static volatile int keepRunning = 1;

void intHandler(int dummy) {
    keepRunning = 0;
}

void printPrompt(){
    printf("%s", prompt);
}

void trimEndingNewline(char *line){
	size_t length = strlen(line);
	if(line[length-1] == '\n')
		line[length-1] = '\0';
}

void parseCommands(char *line, char *commands){
	//char *argument = "hello there";
	//memcpy(commands, argument, 11);
}

int main(void){

	sprintf(prompt, "%s LaSH %% ", getenv("USER"));

    // ignore all signals that should be passed to jobs
    signal (SIGINT, SIG_IGN);
    signal (SIGQUIT, SIG_IGN);
   	signal (SIGTSTP, SIG_IGN);
   	signal (SIGTTIN, SIG_IGN);
   	signal (SIGTTOU, SIG_IGN);
   	signal (SIGCHLD, SIG_IGN);

	int linelength =  MAXCOMMANDS * MAXARGS * MAXARGLENGTH;
    char *input = (char*) malloc( linelength );

    strcpy(input, "");

	char commandArray[ MAXCOMMANDS ][ MAXARGS ][ MAXARGLENGTH ];

	// Loop forever. This will be broken if exit is run
    while(1){
        printPrompt();
        fgets(input, linelength, stdin);

		// cut newline off and replace with null
		trimEndingNewline(input);

		parseCommands(input, &commandArray[0][0][0]);

		printf("%s", commandArray[0][0]);

		if(strcmp("exit", input) == 0)
			break;

		if(strcmp("pwd", input) == 0){

		}

    }

    exit(0);
}
