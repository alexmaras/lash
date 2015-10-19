#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <linux/limits.h>

#define NUMINBUILTCOMMANDS 4
#define MAXPROMPT 40

#include "lashparser.h"
#include "lash.h"


pid_t runningPid;
int acceptInterrupt;
char prompt[MAXPROMPT];

static int maxcommands;
static int maxargs;
static int maxarglength;
static int maxpromptlength;

static char *inbuiltCommands[] = {
	"cd",
	"exit",
	"prompt",
	"pwd"
};

int runShellCommand(struct LashParser *parser, struct Command *command){
	if(strcmp("exit", command->args[0]) == 0){
		return 0;
	}
	if(strcmp("cd", command->args[0]) == 0){
		if(command->argNum == 1){
			chdir(getenv("HOME"));
		} else {
			int status = chdir(command->args[1]);
			char *error = strerror(errno);
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
	if(strcmp("pwd", command->args[0]) == 0){
		char buf[PATH_MAX];
		printf("pwd: %s\n", getcwd(buf, PATH_MAX));
	}
	return 1;
}


int runCommand(struct Command *command, int input){
	int fd[2];
	int redirectOut = -1;
	int redirectIn = -1;
	int pipeout = 0;
	int background = 0;


	//-------------------------------------------------
	//   Evaluate the redirects and pipes
	//-------------------------------------------------
	if(command->redirectOut != NULL){
		redirectOut = creat(command->redirectOut, 0644);
		if(redirectOut == -1){
			printf("File Error: %s\n", strerror(errno));
			return -1;
		}
	}
	if(command->redirectIn != NULL){
		redirectIn = open(command->redirectIn, O_RDONLY);
		if(redirectIn == -1){
			printf("File Error: %s\n", strerror(errno));
			return -1;
		}
	}
	if(command->symbolAfter == '|'){
		pipeout = 1;
		pipe(fd);
	}
	if(command->symbolAfter == '&'){
		background = 1;
	}
	//------------------------------------------------ END


	runningPid = fork();
	if(runningPid < 0){
		printf("Fork error: %s\n", strerror(errno));
	}
	else if(runningPid == 0){
		// child process
		// redirect takes precendence
		if(redirectOut != -1){
			dup2(redirectOut, STDOUT_FILENO);
			close(redirectOut);
		}
		else if(pipeout){
			dup2(fd[1], STDOUT_FILENO);
			close(fd[1]);
		}

		if(redirectIn != -1){
			dup2(redirectIn, STDIN_FILENO);
			close(redirectIn);
		}
		else if(input != -1){
			dup2(input, STDIN_FILENO);
			close(input);
		}

		execvp(command->args[0], command->args);
		char* error = strerror(errno);
		printf("LaSH: %s: %s\n", command->args[0], error);
		exit(1);
	} else {
		if(pipeout)
			close(fd[1]);
		close(redirectOut);
		close(redirectIn);
		close(input);

		if(!background){
			int commandStatus;
			acceptInterrupt = 1;
			waitpid(runningPid, &commandStatus, 0);
			acceptInterrupt = 0;
		}
	}
	return pipeout ? fd[0] : -1;
}

int inArray(char *needle, char*haystack[], int length){
	int i;
	for(i = 0; i < length; i++){
		if(strcmp(needle, haystack[i]) == 0)
			return 1;
	}
	return 0;
}

int executeCommand(struct LashParser *parser){
	int i;
	int nextinput = -1;
	for(i = 0; i < parser->commandNum; i++){
		struct Command *command = parser->commands[i];
		int shellCommand = inArray(command->args[0], inbuiltCommands, NUMINBUILTCOMMANDS);
		if(shellCommand){
			return runShellCommand(parser, command);
		}
		else if(strcmp(command->args[0], "") != 0){
			nextinput = runCommand(command, nextinput);
		}
	}
	return 1;
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
	int cont = 1;
	int status;
    while(cont){
		acceptInterrupt = 0;

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
