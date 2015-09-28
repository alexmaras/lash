#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>

#define MAXCOMMANDS 100
#define MAXARGS 1000
#define MAXARGLENGTH 100
#define MAXINPUT ( MAXCOMMANDS * MAXARGS * MAXARGLENGTH )

pid_t runningPid;
char prompt[80];

void printPrompt(){
    printf("%s", prompt);
}

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

bool insideQuotes(int index, int quotes[][2], int numberOfQuotePairs){
    int i;
    for(i = 0; i < numberOfQuotePairs; i++){
        if(quotes[i][0] < index && index < quotes[i][1]){
            return true;
        }
    }
    return false;
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

    if(line[length-1] == '\n')
		line[length-1] = '\0';

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

void cleanString(char *line, int quotes[][2], int numberOfQuotePairs){
	int length = strlen(line);
	int j = 0;
	int i = 0;

    if(line[length-1] == '\n')
		line[length-1] = '\0';

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

	strcpy(line, newline);

}

void parseCommand(char *command, char **args, int quotes[][2], int numberOfQuotePairs){
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
            tempString[j] = '\0';
            stripStartAndEndSpacesAndSemiColons(tempString);

            if((tempString[0] == '"' && tempString[strlen(tempString)-1] == '"') || (tempString[0] == '\'' && tempString[strlen(tempString)-1] == '\'')){
                int start = 1;
                int end = strlen(tempString)-1;
                j = 0;
                for(i = start; i < end; i++){
                    tempString[j] = tempString[i];
                    j++;
                }
            }
            tempString[j] = '\0';
            args[argnum] = tempString;
            argnum++;
            copyIndex++;
        }
    }
}

int splitCommands(char *line, char **commands, int quotes[][2], int numberOfQuotePairs){
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
            cleanString(tempString, quotes, numberOfQuotePairs);
            commands[commandnum] = tempString;
            commandnum++;
            copyIndex++;
        }
    }
	return commandnum;
}


bool indexNotInArray(int array[][2], int arrayIndex, int foundCharIndex){
    int index;
    for(index = 0; index < arrayIndex; index++){
        if(array[index][0] == foundCharIndex || array[index][1] == foundCharIndex){
            return false;
        }
    }
    return true;
}

int findQuoteLocations(char *line, int quotes[][2]){
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
                // quit immediately
            }

            quotes[quoteIndex][0] = index;
            quotes[quoteIndex][1] = secondIndex;
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

        int quotes[MAXCOMMANDS][2];


        int foundAmount = findQuoteLocations(input, quotes);
		cleanString(input, quotes, foundAmount);


		int commandnum = splitCommands(input, commandArray, quotes, foundAmount);

		if(commandnum != 0){
			int i = 0;
			for(i=0; i<commandnum; i++){
				emptyArray(args, MAXARGS);
				parseCommand(commandArray[i], args, quotes, foundAmount);
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
