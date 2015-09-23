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

void parseCommands(char *line, char *commands){
	//char *argument = "hello there";
	//memcpy(commands, argument, 11);

	const char semicol[2] = ";&";
	char *token;

	token = strtok(line, semicol);
	while(token != NULL){
		trimExtraWhitespace(token);
		printf("%s\n", token);
		token = strtok(NULL, semicol);
	}

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
		trimExtraWhitespace(input);

		parseCommands(input, &commandArray[0][0][0]);

		printf("%s\n", commandArray[0][0]);

		if(strcmp("exit", input) == 0)
			break;

		if(strcmp("pwd", input) == 0){

		}

    }

    exit(0);
}
