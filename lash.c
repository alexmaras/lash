#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>


#define MAXCOMMANDS 100
#define MAXARGS 1000
#define MAXARGLENGTH 100
#define MAXINPUT ( MAXCOMMANDS * MAXARGS * MAXARGLENGTH )

pid_t runningPid;
char prompt[80];

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

void parseCommand(char *command, char **args){

	const char space[2] = " ";
	int argnum = 0;
	char *argument, *argumentPointer;

	argument = strtok_r(command, space, &argumentPointer);
	while(argument != NULL){
		args[argnum] = argument;
		argument = strtok_r(NULL, space, &argumentPointer);
		argnum++;
	}
}

int splitCommands(char *line, char **commands){
	const char delim[3] = ";&";

	char *commandPointer;
	char *command;
	int commandnum = 0;

	command = strtok_r(line, delim, &commandPointer);
	while(command != NULL){
		trimExtraWhitespace(command);
		trimEndingNewline(command);
		commands[commandnum] = command;
		command = strtok_r(NULL, delim, &commandPointer);
		commandnum++;
	}

	return commandnum;
}


void executeCommand(char **args){
	runningPid = fork();

	if(runningPid == -1){
		printf("for error: %s\n", strerror(errno));
		return;
	}

	else if(runningPid == 0){
		// child process
		execvp(args[0], args);

		char* error = strerror(errno);
		printf("shell: %s: %s\n", args[0], error);
		return;
	} else {
		// parent process
		int commandStatus;
		waitpid(runningPid, &commandStatus, 0);
		return;
	}
}

void emptyArray(char **array, int length){
	int delete = 0;
	for(delete = 0; delete < length; delete++){
		array[delete] = NULL;
	}
}

void sighandler(int signum){
    if(signum == SIGINT){
        kill(runningPid, SIGINT);
    }

    printf("\n");
}

int main(void){

	sprintf(prompt, "%s LaSH %% ", getenv("USER"));

    // ignore all signals that should be passed to jobs
    signal (SIGINT, sighandler);
    signal (SIGQUIT, SIG_IGN);
   	signal (SIGTSTP, SIG_IGN);
   	signal (SIGTTIN, SIG_IGN);
   	signal (SIGTTOU, SIG_IGN);
   	signal (SIGCHLD, SIG_IGN);

	char *input = (char*) malloc( MAXINPUT );
	char *commandArray[ MAXCOMMANDS ];
	char *args[ MAXARGS ];

	// Loop forever. This will be broken if exit is run
    while(1){
		strcpy(input, "");

		emptyArray(commandArray, MAXCOMMANDS);

        printPrompt();
        fgets(input, MAXINPUT, stdin);

		// cut newline off and replace with null
		trimEndingNewline(input);
		trimExtraWhitespace(input);
		int commandnum = splitCommands(input, commandArray);

		if(commandnum != 0){
			int i = 0;
			for(i=0; i<commandnum; i++){
				emptyArray(args, MAXARGS);
				parseCommand(commandArray[i], args);
				if(strcmp("exit", args[0]) == 0)
					break;
				if(strcmp(args[0], "") != 0)
					executeCommand(args);
			}

			if(strcmp("exit", args[0]) == 0)
				break;
		}
    }

    exit(0);
}
