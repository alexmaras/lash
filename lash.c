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
#include <linux/limits.h>

#include "lashparser.h"
#include "lash.h"

#define MAXPROMPT 40

pid_t runningPid;
bool acceptInterrupt;
char prompt[MAXPROMPT];

static int maxcommands;
static int maxargs;
static int maxarglength;
static int maxpromptlength;

bool runShellCommand(struct LashParser *parser, struct Command *command){
	if(strcmp("exit", command->args[0]) == 0){
		return false;
	}
	if(strcmp("cd", command->args[0]) == 0){
		if(command->argNum == 1){
			chdir(getenv("HOME"));
		} else {
			char *path = malloc(parser->maxinput);
			strcpy(path, command->args[1]);
			if(command->args[1][0] == '~'){
				strcpy(path, getenv("HOME"));
				char *buffer = malloc(sizeof(char) * parser->maxinput);
				memcpy(buffer, &(command->args[1][1]), strlen(command->args[1])-1);
				strcat(path, buffer);
				free(buffer);
			}
			int status = chdir(path);
			char *error = strerror(errno);
			free(path);
			if(status == -1)
				printf("%s\n", error);
		}
	}
	if(strcmp("prompt", command->args[0]) == 0){
		if(command->argNum == 1)
			printf("No prompt given\n");
		else {
			if(strlen(command->args[1]) > maxpromptlength-1){
				printf("The prompt can only be %d characters long\n", maxpromptlength);
			}
			else {
				strcpy(prompt, command->args[1]);
				strcat(prompt, " ");
			}
		}
	}
	return true;
}


int runCommand(struct Command *command, int input){
	int fd[2];
	fd[0] = STDIN_FILENO;
	fd[1] = STDOUT_FILENO;
	//char readbuffer[80];
	if(command->symbolAfter == '|'){
		pipe(fd);
	}

	runningPid = fork();
	if(runningPid == -1){
		printf("for error: %s\n", strerror(errno));
	}
	else if(runningPid == 0){
		// child process


		//printf("nextchar: %c\n", command->symbolAfter);
		//printf("redirects: in: %s, out: %s\n", command->redirectIn, command->redirectOut);
		//printf("file desc: %d, %d\n", input, fd[1]);
		//printf("stdout: %d. stdin: %d\n", STDOUT_FILENO, STDIN_FILENO);

		if(fd[1] != STDOUT_FILENO){
			dup2(fd[1], STDOUT_FILENO);
		}
		if(input != STDIN_FILENO){
			dup2(input, STDIN_FILENO);
		}

		execvp(command->args[0], command->args);
		char* error = strerror(errno);
		printf("LaSH: %s: %s\n", command->args[0], error);
		exit(1);
	} else {
		if(fd[1] != STDOUT_FILENO)
			close(fd[1]);
		// parent process
		int commandStatus;
		acceptInterrupt = true;
		waitpid(runningPid, &commandStatus, 0);
		acceptInterrupt = false;
		//printf("finished!\n");
	}
	return fd[0];
}

bool executeCommand(struct LashParser *parser){
	int i;
	int nextinput = 0;
	for(i = 0; i < parser->commandNum; i++){
		struct Command *command = parser->commands[i];
		bool shellCommand = (strcmp("exit", command->args[0]) == 0 || strcmp("cd", command->args[0]) == 0 || strcmp("prompt", command->args[0]) == 0);
		if(shellCommand){
			return runShellCommand(parser, command);
		}
		else if(strcmp(command->args[0], "") != 0){
			nextinput = runCommand(command, nextinput);
		}
	}
	return true;
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
		if(acceptInterrupt)
			kill(runningPid, SIGINT);
        //kill(runningPid, SIGINT);
    }
	rl_free_line_state();
	rl_cleanup_after_signal();
}


void runLash(int command, int args, int arglength, int promptlength){

	maxcommands     = command;
	maxargs         = args;
	maxarglength    = arglength;
	maxpromptlength = promptlength;

	sprintf(prompt, "%s LaSH %% ", getenv("USER"));

    // ignore all signals that should be passed to jobs
    signal (SIGINT, sighandler);
    signal (SIGQUIT, SIG_IGN);
   	signal (SIGTSTP, SIG_IGN);
   	signal (SIGTTIN, SIG_IGN);
   	signal (SIGTTOU, SIG_IGN);
   	signal (SIGCHLD, SIG_IGN);

	stifle_history(100);

	struct LashParser *lash = newLashParser(maxcommands, maxargs, maxarglength);


	//printf("defines: %d, %d, %d\n", PIPE, REDIRECTBACKWARD, REDIRECTFORWARD);

	// Loop forever. This will be broken if exit is run
	bool cont = true;
	int status;
    while(cont){
		acceptInterrupt = false;

        char *input = readline(prompt);
		if(strcmp(input, "") != 0){
	        add_history(input);
		}
		status = buildCommand(lash, input);

		if(status == VALID)
			cont = executeCommand(lash);
		else if(status == QUOTE_MISMATCH)
			printf("Error: Quote Mismatch\n");

        free(input);
		clearParser(lash);
    }

	free(lash->commands);
	free(lash);

    exit(0);
}
