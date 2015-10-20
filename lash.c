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

/**
 * Runs shell specific commands.
 * The following commands are acceptable:
 *    - exit
 *    - cd
 *    - prompt
 *    - pwd
 * Note that given the commands "exit" and "pwd", any arguments after the first
 * are completely ignored.
 * Given the commands "prompt" and "cd", only the first argument after the command
 * itself will be considered and any later arguments discarded
 *
 * @param  parser  A pointer to an initialiased LashParser struct
 * @param  command A pointer to an initialiased and populated command struct
 * @return         0 if the command was exit, -1 if there was an error, 1 if the command was executed successfully
 */
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


/**
 * Runs commands that are not shell specific.
 * This takes a command struct which should be fully populated
 * and an integer. The input integer is used internally to specify the input file descriptor
 * for the process - i.e. which file descriptor to read from, if any.
 * If a file specifier is not going to be specified, the input should be set to -1
 * @param  command A pointer to an initialiased and populated command struct
 * @param  input   An integer which represents a file descriptor
 * @return         The input file descriptor for the next command if this command contained a pipe
 */
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
		// -----------------------------------------------
		//  Setting up file descriptors
		//  ----------------------------------------------
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
		//------------------------------------------------ END

		// Execute the command
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

/**
 * Checks whether the given string is represented verbatim in the given array of
 * strings. The length parameter is the amount of string stored in the haystack.
 * @param  needle   The string to search for
 * @param  haystack The array of string to search within
 * @param  length   The length of the haystack
 * @return          1 if found, 0 if not
 */
int inArray(char *needle, char*haystack[], int length){
	int i;
	for(i = 0; i < length; i++){
		if(strcmp(needle, haystack[i]) == 0)
			return 1;
	}
	return 0;
}

/**
 * Evaluates each command stored in the parser struct and exeutes either the runShellCommand
 * function or the runCommand function.
 * @param  parser A constructed LashParser struct with fully populated commands
 * @return        0 if a command was "exit", 1 otherwise
 */
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

/**
 * Handles the signals to programs. Currently functions only for SIGINTS when
 * a program is being executed
 * @param signum The signal number
 */
void sighandler(int signum){
    if(signum == SIGINT){
		if(acceptInterrupt)
			kill(runningPid, SIGINT);
    }
	rl_free_line_state();
	rl_cleanup_after_signal();
}

/**
 * Runs the lash program. The given parameters allow the user to specify the
 * limits the program should operate within.
 *
 * @param command      The maximum amount of commands to allow
 * @param args         The maximum number of arguments within each command
 * @param arglength    The maximum length of each argument
 * @param promptlength The maximum length of the prompt on the command line
 */
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
 //   	signal (SIGCHLD, sighandler);

	stifle_history(100);

	struct LashParser *lash = newLashParser(maxcommands, maxargs, maxarglength);

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
