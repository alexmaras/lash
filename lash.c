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


pid_t runningPid;
char prompt[MAXPROMPT];

bool isEscaped(char *line, int index){
    if(line[index-1] == '\\'){
        return true;
    }
    return false;
}

bool followedBySemiColon(char *line, int index){
    int i = index+1;
    while(line[i] == ' '){
        i++;
    }
    if(line[i] == ';'){
        return true;
    }

    return false;
}

int insideQuotes(int index, int quotes[][3], int numberOfQuotePairs){
    int i;
    for(i = 0; i < numberOfQuotePairs; i++){
        if(quotes[i][0] < index && index < quotes[i][1]){
            return quotes[i][2];
        }
    }
    return 0;
}

bool atStart(int index, char *line){
    int ending = 0;
    int i;

    for(i = index-1; i >= ending; i--){
        if(line[i] != ' ' && line[i] != ';'){
            return false;
        }
    }
    return true;
}


bool atEnd(int index, char *line){
    int ending = strlen(line);
    int i;

    for(i = index+1; i < ending; i++){
        if(line[i] != ' ' && line[i] != ';'){
            return false;
        }
    }
    return true;
}
void stripStartAndEndSpacesAndSemiColons(char *line){
	int length = strlen(line);
	int j = 0;
	int i = 0;

	char *newline = (char*) malloc( length );

	for(i = 0; i<length; i++){
		if(!(line[i] == ';' &&
                (
                    (atStart(i, line)) ||	// semicolon at start
                    (atEnd(i, line))     	// semicolon at end
                )
            )
            &&
            !(line[i] == ' ' &&
                (
			        (atStart(i, line)) ||	// space at start
			        (atEnd(i, line))    	// space at end
			    )
            )

		){
			newline[j] = line[i];
			j++;
		}
	}
    newline[j] = '\0';

	strcpy(line, newline);

}

void cleanString(char *line){
	// get locations of quotes
	int quotes[MAXCOMMANDS][3];
	int numberOfQuotePairs = findQuoteLocations(line, quotes);

	int length = strlen(line);
	int j = 0;
	int i = 0;

	char *newline = (char*) malloc( length );

	for(i = 0; i<length; i++){
		if( insideQuotes(i, quotes, numberOfQuotePairs) ||
            (
                !(line[i] == ';' &&
                    (
                        (atStart(i, line)) ||	// semicolon at start
                        (atEnd(i, line))   ||	// semicolon at end
                        (line[i+1] == ';') ||	// semicolon followed by semicolon
                        (j == 0) 				// first character of the new string
                    )
                )
                &&
                !(line[i] == ' ' &&
                    (
				        (atStart(i, line)) ||	// space at start
				        (atEnd(i, line))   ||	// space at end
				        (line[i+1] == ' ') ||	// space followed by space
				        (j == 0) 				// first character of the new string
			        )
                )
            )
		){
			newline[j] = line[i];
			j++;
		}
	}

    newline[j] = '\0';

	strcpy(line, newline);
}

void removeEscapeSlashesAndQuotes(char *line){

	int quotes[MAXCOMMANDS][3];
	int numberOfQuotePairs = findQuoteLocations(line, quotes);

	char *tempString = (char*) malloc( MAXARGLENGTH );
	int i, j = 0, nextchar = -1;

	for(i = 0; i < strlen(line); i++){
		nextchar = -1;
		if(i != (strlen(line) - 1)){
			if(line[i+1] == '\''){
				nextchar = 1;
			} else if(line[i+1] == '\"'){
				nextchar = 2;
			}
		}
		int quoteType = insideQuotes(i, quotes, numberOfQuotePairs);
		if(line[i] != '\\' || isEscaped(line, i) || (quoteType != 0 && ( nextchar != quoteType ))){
			tempString[j] = line[i];
			j++;
		}
	}


    if((line[0] == '"' && line[strlen(line)-1] == '"') || (line[0] == '\'' && line[strlen(line)-1] == '\'')){
    	int start = 1;
        int end = strlen(tempString)-1;
        j = 0;
        for(i = start; i < end; i++){
        	tempString[j] = tempString[i];
            j++;
        }
    }

	tempString[j] = '\0';
	strcpy(line, tempString);
	free(tempString);
}

int parseCommand(char *command, char **args){
	int quotes[MAXCOMMANDS][3];
	int numberOfQuotePairs = findQuoteLocations(command, quotes);

	int argnum = 0;
    int i;
    char currentChar = ' ';
    int copyIndex = 0;

    for(i = 0; i < strlen(command); i++){
        bool lastChar = (i == strlen(command)-1);
        currentChar = command[i];
        if(lastChar || (currentChar == ' ' && !isEscaped(command, i) && !insideQuotes(i, quotes, numberOfQuotePairs))){
            char *tempString = (char*) malloc( MAXARGLENGTH );
            int j = 0;
            while((copyIndex < i && !lastChar) || (copyIndex <= i && lastChar) ){
                tempString[j] = command[copyIndex];
                j++;
                copyIndex++;
            }
            stripStartAndEndSpacesAndSemiColons(tempString);
			tempString[j] = '\0';

			removeEscapeSlashesAndQuotes(tempString);
            args[argnum] = tempString;

            argnum++;
            copyIndex++;
        }
    }
	return argnum;
}

int splitCommands(char *line, char **commands){

	int quotes[MAXCOMMANDS][3];
	int numberOfQuotePairs = findQuoteLocations(line, quotes);

	int commandnum = 0;
    int i;
    char currentChar = ' ';
    int copyIndex = 0;

    for(i = 0; i < strlen(line); i++){
        bool lastChar = (i == strlen(line)-1);
        currentChar = line[i];
        if(lastChar || ((currentChar == '&' || currentChar == ';') && !followedBySemiColon(line, i) && !isEscaped(line, i) && !insideQuotes(i, quotes, numberOfQuotePairs))){
            char *tempString = (char*) malloc( MAXINPUT );
            int j = 0;
            while((copyIndex < i && !lastChar) || (copyIndex <= i && lastChar) ){
                tempString[j] = line[copyIndex];
                j++;
                copyIndex++;
            }
            tempString[j] = '\0';
            cleanString(tempString);
            commands[commandnum] = tempString;
            commandnum++;
            copyIndex++;
        }
    }
	return commandnum;
}


bool indexNotInArray(int array[][3], int arrayIndex, int foundCharIndex){
    int index;
    for(index = 0; index < arrayIndex; index++){
        if(array[index][0] == foundCharIndex || array[index][1] == foundCharIndex){
            return false;
        }
    }
    return true;
}

int findQuoteLocations(char *line, int quotes[][3]){
    int index, secondIndex, quoteIndex = 0;
    char currentChar = ' ';
    int linelength = strlen(line);
    bool foundMatch = false;

    for(index = 0; index < linelength; index++){
        currentChar = line[index];

        if((currentChar == '\"' || currentChar == '\'') && indexNotInArray(quotes, quoteIndex, index) && !isEscaped(line, index)){
            for(secondIndex = index+1; secondIndex < linelength; secondIndex++){
                foundMatch = false;
                if(line[secondIndex] == currentChar && indexNotInArray(quotes, quoteIndex, index) && !isEscaped(line, secondIndex)){
                    foundMatch = true;
                    break;
                }
            }

            if(!foundMatch){
				return -1;
                // quit immediately
            }

			// 1st element = 1st index of quotes
			// 2nd element = 2nd index of quotes
			// 3rd element = 1 for single quote, 2 for double quote
            quotes[quoteIndex][0] = index;
            quotes[quoteIndex][1] = secondIndex;
			quotes[quoteIndex][2] = ( currentChar == '\'' ? 1 : 2 );
            quoteIndex++;
            index = secondIndex+1;
        }
    }
    return quoteIndex;
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

	// char *input = (char*) malloc( MAXINPUT );
	char *commandArray[ MAXCOMMANDS ];
	char *args[ MAXARGS ];

	// Loop forever. This will be broken if exit is run
    while(1){

        char *input = readline(prompt);
        add_history(input);

		int argnum = 0;

		int quotes[MAXCOMMANDS][3];
		int numberOfQuotes = findQuoteLocations(input, quotes);

		if(numberOfQuotes == -1){
			printf("Quotes mismatch\n");
		} else {
			cleanString(input);
			emptyArray(commandArray, MAXCOMMANDS);
			int commandnum = splitCommands(input, commandArray);
			if(commandnum != 0){
				int i;
				for(i=0; i<commandnum; i++){
					emptyArray(args, MAXARGS);
					argnum = parseCommand(commandArray[i], args);
					bool shellCommand = (strcmp("exit", args[0]) == 0 || strcmp("cd", args[0]) == 0 || strcmp("prompt", args[0]) == 0);
					if(shellCommand)
						break;
					if(strcmp(args[0], "") != 0)
						executeCommand(args);
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
    }

    exit(0);
}
