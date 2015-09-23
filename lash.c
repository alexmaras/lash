#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#define MAXCOMMANDS 100
#define MAXARGS 1000
#define MAXARGLENGTH 100
#define MAXINPUT ( MAXCOMMANDS * MAXARGS * MAXARGLENGTH )


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

void trimExtraWhitespace(char *line){

	int length = strlen(line);
	int j = 0;
	int i = 0;
	char *newline = (char*) malloc( length );

	for(i = 0; i<length; i++){
		if( !(line[i] == ' ' && (
				(i == 0)           ||	// space at start
				(i == length-1)    ||	// space at end
				(line[i+1] == ' ') ||	// space followed by space
				(j == 0) 				// first character of the new string
			))
		){
			newline[j] = line[i];
			j++;
		}
	}

	strcpy(line, newline);

}
int * parseCommands(char *line, char *commands){
	//char *argument = "hello there";
	//memcpy(commands, argument, 11);

	const char delim[3] = ";&";
	const char space[2] = " ";
	char *command, *argument;
	int commandnum = 0;
	int argnum = 0;
	static int commargs[ MAXCOMMANDS ];

	memset(commargs, -1, MAXCOMMANDS);

	char *commandPointer, *argumentPointer;

	char *commandCopy = (char*) malloc( MAXINPUT );

	command = strtok_r(line, delim, &commandPointer);
	while(command != NULL){
		trimExtraWhitespace(command);

		strcpy(commandCopy, command);

		argument = strtok_r(commandCopy, space, &argumentPointer);
		while(argument != NULL){
			//memcpy(commands + ((commandnum * MAXCOMMANDS) + (argnum * MAXARGS)), argument, strlen(argument));
			strcpy(commands + ((commandnum * MAXARGS * MAXARGLENGTH) + (argnum * MAXARGLENGTH)), argument);
			argument = strtok_r(NULL, space, &argumentPointer);
			argnum++;
		}

		commargs[commandnum] = argnum;

		commandnum++;
		argnum = 0;
		command = strtok_r(NULL, delim, &commandPointer);
	}
	return commargs;
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

	char *input = (char*) malloc( MAXINPUT );
    strcpy(input, "");

	char commandArray[ MAXCOMMANDS ][ MAXARGS ][ MAXARGLENGTH ];

	// Loop forever. This will be broken if exit is run
    while(1){
        printPrompt();
        fgets(input, MAXINPUT, stdin);

		// cut newline off and replace with null
		trimEndingNewline(input);
		trimExtraWhitespace(input);

		int *commargs = parseCommands(input, &commandArray[0][0][0]);

		int command = 0;
		while(commargs[command] != -1){
			int argument;
			for(argument = 0; argument<commargs[command]; argument++){
				printf("%s\n", commandArray[command][argument]);
				//run command with arguments
			}
			command++;
		}

		if(strcmp("exit", input) == 0)
			break;

		if(strcmp("pwd", input) == 0){

		}

    }

    exit(0);
}
