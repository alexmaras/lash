#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <readline/history.h>


#define MAXCOMMANDS 100
#define MAXARGS 1000
#define MAXARGLENGTH 100
#define MAXINPUT ( MAXCOMMANDS * MAXARGS * MAXARGLENGTH )
#define MAXPROMPT 40

#include "lash.h"
#include "lashparser.c"

pid_t runningPid;
char prompt[MAXPROMPT];

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
		printf("\nshell: %s: %s\n", args[0], error);
		exit(1);
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
		free(array[delete]);
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

	//char *commandArray[ MAXCOMMANDS ];
	//char *args[ MAXARGS ];

	stifle_history(100);


	// Loop forever. This will be broken if exit is run
    while(1){

		char *commandArray[ MAXCOMMANDS ];
		char *args[ MAXARGS ];

        char *input = readline(prompt);
		if(strcmp(input, "") != 0){
	        add_history(input);
		}

		int argnum = 0;
		int commandnum = 0;
		int quotes[MAXCOMMANDS][3];
		int numberOfQuotes = findQuoteLocations(input, quotes);

		if(numberOfQuotes == -1){
			printf("Quotes mismatch\n");
		} else {
			cleanString(input);
			commandnum = splitCommands(input, commandArray);
			if(commandnum != 0){
				int i;
				for(i=0; i<commandnum; i++){
					argnum = parseCommand(commandArray[i], args);
					bool shellCommand = (strcmp("exit", args[0]) == 0 || strcmp("cd", args[0]) == 0 || strcmp("prompt", args[0]) == 0);
					if(shellCommand)
						break;
					if(strcmp(args[0], "") != 0){
						executeCommand(args);
					}
				}

				if(strcmp("exit", args[0]) == 0)
					break;
				if(strcmp("cd", args[0]) == 0){
					if(argnum == 1){
						chdir(getenv("HOME"));
					} else {
						char path[MAXARGLENGTH] = "";
						strcpy(path, args[1]);
						if(args[1][0] == '~'){
							strcpy(path, getenv("HOME"));
							char buffer[MAXARGLENGTH] = "";
							memcpy(buffer, &args[1][1], strlen(args[1])-1);
							strcat(path, buffer);
						}
						int status = chdir(path);
						char *error = strerror(errno);
						if(status == -1)
							printf("%s\n", error);
					}
				}
				if(strcmp("prompt", args[0]) == 0){
					if(argnum == 1)
						printf("No prompt given\n");
					else {
						if(strlen(args[1]) > MAXPROMPT-1){
							printf("The prompt can only be %d characters long\n", MAXPROMPT);
						}
						else {
							strcpy(prompt, args[1]);
							strcat(prompt, " ");
						}
					}
				}
			}
		}
        free(input);
		emptyArray(commandArray, commandnum);
		emptyArray(args, argnum);
    }

    exit(0);
}
