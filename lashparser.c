#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "lashparser.h"

struct Command *newCommand(struct LashParser *parser){
	struct Command *newCommand = (struct Command *)malloc(sizeof(struct Command));
	newCommand->args = malloc(sizeof(char) * parser->maxargs);
	newCommand->pipeOrRedirect = malloc(sizeof(int) * parser->maxargs);
	newCommand->argNum = 0;
	return newCommand;
}

struct LashParser *newLashParser(int commands, int args, int arglength){
	struct LashParser *parser = (struct LashParser *)malloc(sizeof(struct LashParser));
	parser->maxcommands = commands;
	parser->maxargs = args;
	parser->maxarglength = arglength;
	parser->maxinput = commands * args * arglength;

	parser->commands = malloc(sizeof(struct Command) * commands);

	return parser;
}

void clearParser(struct LashParser *parser){
	int i;
	int j;
	for(i = 0; i < parser->commandNum; i++){
		for(j = 0; j < parser->commands[i]->argNum; j++){
			free(parser->commands[i]->args[j]);
		}
		free(parser->commands[i]->pipeOrRedirect);
		free(parser->commands[i]->command);
		free(parser->commands[i]->args);
		free(parser->commands[i]);
	}

}

bool isEscaped(char *line, int index){
	if(index == 0)
		return false;
    if(line[index-1] == '\\' && !isEscaped(line, index-1)){
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
	free(newline);
}

void cleanString(struct LashParser *parser, char *line){
	// get locations of quotes
	int quotes[parser->maxcommands][3];
	int numberOfQuotePairs = findQuoteLocations(line, quotes);

	int length = strlen(line);
	int j = 0;
	int i = 0;

	char *newline = (char*) malloc( parser->maxargs * parser->maxarglength );

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
						(line[i+1] == '|') ||
						(line[i+1] == '<') ||
						(line[i+1] == '>') ||
						(line[i-1] == '|') ||
						(line[i-1] == '<') ||
						(line[i-1] == '>') ||
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

	free(newline);
}

void removeEscapeSlashesAndQuotes(struct LashParser *parser, char *line){

	int quotes[parser->maxcommands][3];
	int numberOfQuotePairs = findQuoteLocations(line, quotes);

	char *tempString = (char*) malloc( parser->maxarglength );
	int i, k, j = 0, nextchar = -1;
	bool enclosingQuote;

	for(i = 0; i < strlen(line); i++){

		nextchar = -1;
		if(i != (strlen(line) - 1)){
			if(line[i+1] == '\''){
				nextchar = 1;
			} else if(line[i+1] == '\"'){
				nextchar = 2;
			}
		}

		// if the current character (i) is in the quotes array, it must be removed
		enclosingQuote = false;
		for(k = 0; k < numberOfQuotePairs; k++){
			if(quotes[k][0] == i || quotes[k][1] == i)
				enclosingQuote = true;
		}

		int quoteType = insideQuotes(i, quotes, numberOfQuotePairs);

		// copy the character to the new string IF
		//     the character is NOT one of an enclosing quote pair AND
		//
		//     the character is either escaped, not a backslash OR is a backslash and is NOT in front of the same type of enclosing quote
		//
		// This sounds complicated but examples are easier:
		//
		// This string:    "hi\" th"ere'You smelly\\ sock'\"
		//
		// removes the quotes locates before hi and in the middle of there (as they are part of an enclosing quote pair),
		// removes the backslash after hi as it is escaping a double quote inside of a double quote pair
		// removes the single quotes around  :::You smelly sock::: as they are part of an enclosing quote pair
		// keeps both backlashes after smelly as they are within enclosing quotes
		// keeps the final double quote as it is escaped
		//
		// Note that currently, double escaping backslashes before a quote will count as the quote being escaped.
		bool copyCharacter = (
			!enclosingQuote &&
			(
				line[i] != '\\' ||
				isEscaped(line, i) ||
				(quoteType != 0 && (nextchar != quoteType))
			)
		);

		if(copyCharacter){
			tempString[j] = line[i];
			j++;
		}

	}

	tempString[j] = '\0';
	strcpy(line, tempString);
	free(tempString);
}

int copyString(char *copyTo, char *copyFrom, int startAt, int endAt){
	int i = 0;
	while(startAt < endAt){
		copyTo[i] = copyFrom[startAt];
		i++;
		startAt++;
	}
	copyTo[i] = '\0';
	return startAt+1;
}


int parseCommand(struct LashParser *parser, struct Command *commData){
	char *command = commData->command;

	// Get the quote locations for the current command
	int quotes[parser->maxcommands][3];
	int numberOfQuotePairs = findQuoteLocations(command, quotes);

    int i;
	int argnum = 0;
    int copyIndex = 0;
	int pipenum = 0;
	int redirectnum = 0;

	int pipeIndexes[parser->maxargs];
	int redirectIndexes[parser->maxargs][2];

    char currentChar;

	pipenum = findPipes(parser, command, pipeIndexes);
	redirectnum = findRedirects(parser, command, redirectIndexes);
	printf("redirects: %d\npipes: %d\n", redirectnum, pipenum);
	printf("command: %s\n", command);
    for(i = 0; i < strlen(command); i++){
		currentChar = command[i];
        bool lastChar = (i == strlen(command)-1);

		// if this is the last character OR:
		//     the current character is a space that is NOT escaped AND is NOT in quotes
		// We have found a break and should copy that section into tempString
		bool foundBreak = (
			lastChar ||
			(
				(currentChar == ' ' || currentChar == '|' || currentChar == '<' || currentChar == '>') &&
				!isEscaped(command, i) &&
				!insideQuotes(i, quotes, numberOfQuotePairs)
			)
		);
		if(foundBreak){
			if(currentChar == '|')
				commData->pipeOrRedirect[argnum] = PIPE;
			else if(currentChar == '<')
				commData->pipeOrRedirect[argnum] = REDIRECTBACKWARD;
			else if(currentChar == '>')
				commData->pipeOrRedirect[argnum] = REDIRECTFORWARD;
			else
				commData->pipeOrRedirect[argnum] = 0;

			printf("pipe/redir: %d\n", commData->pipeOrRedirect[argnum]);
			commData->args[argnum] = (char*) malloc( (int)(parser->maxarglength * sizeof(char)));

			copyIndex = copyString(commData->args[argnum], command, copyIndex, ( lastChar ? i+1 : i));
	        stripStartAndEndSpacesAndSemiColons(commData->args[argnum]);
			removeEscapeSlashesAndQuotes(parser, commData->args[argnum]);

            argnum++;
			commData->args[argnum] = NULL; // Make sure final arg is always NULL so that execvp handles it correctly
        }
    }
	commData->argNum = argnum;
	//free(tempString);
	return argnum;
}

int foundPipeOrRedirect(int index, int *pipes, int pipenum, int redirects[][2], int redirectnum){
	int i;
	for(i = 0; i < pipenum; i++){
		if(pipes[i] == index){
			return PIPE;
		}
	}

	for(i = 0; i < redirectnum; i++){
		if(redirects[i][0] == index){
			return redirects[i][1]; // WILL RETURN REDIRECTBACKWARD IF BACKWARD, REDIRECTFORWARD IF FORWARD
		}
	}

	return 0;
}

int splitCommands(struct LashParser *parser, char *line){

	cleanString(parser, line);

	int quotes[parser->maxcommands][3];
	int numberOfQuotePairs = findQuoteLocations(line, quotes);

	int commandnum = 0;
    int i;
    char currentChar = ' ';
    int copyIndex = 0;

    for(i = 0; i < strlen(line); i++){
        bool lastChar = (i == strlen(line)-1);
        currentChar = line[i];
        if(lastChar || ((currentChar == '&' || currentChar == ';') && !followedBySemiColon(line, i) && !isEscaped(line, i) && !insideQuotes(i, quotes, numberOfQuotePairs))){
            char *tempString = (char*) malloc(parser->maxinput);
            int j = 0;
            while((copyIndex < i && !lastChar) || (copyIndex <= i && lastChar)){
                tempString[j] = line[copyIndex];
                j++;
                copyIndex++;
            }
            tempString[j] = '\0';
            cleanString(parser, tempString);
			parser->commands[commandnum] = newCommand(parser);
            parser->commands[commandnum]->command = tempString;
            commandnum++;
            copyIndex++;
        }
    }
	parser->commandNum = commandnum;
	return commandnum;
}

int findPipes(struct LashParser *parser, char *line, int *pipeIndexes){

    int quotes[parser->maxcommands][3];
    int numberOfQuotePairs = findQuoteLocations(line, quotes);

	int i;
	int j = 0;
	for(i = 0; i < strlen(line); i++){
		if(line[i] == '|' && !isEscaped(line, i) && !insideQuotes(i, quotes, numberOfQuotePairs)){
			pipeIndexes[j] = i;
			j++;
		}
	}
	return j;
}

int findRedirects(struct LashParser *parser, char *line, int redirectIndexes[][2]){

    int quotes[parser->maxcommands][3];
    int numberOfQuotePairs = findQuoteLocations(line, quotes);

    int i;
    int j = 0;
    for(i = 0; i < strlen(line); i++){
        if((line[i] == '<' || line[i] == '>') && !isEscaped(line, i) && !insideQuotes(i, quotes, numberOfQuotePairs)){
			redirectIndexes[j][0] = i;
			redirectIndexes[j][1] = (line[i] == '<' ? REDIRECTBACKWARD : REDIRECTFORWARD);
            j++;
        }
    }
    return j;
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


void buildCommand(struct LashParser *parser, char *line){
	int quotes[parser->maxcommands][3];
	int numberOfQuotes = findQuoteLocations(line, quotes);
	if(numberOfQuotes == -1){
		printf("Quotes mismatch\n");
	} else {
		splitCommands(parser, line);
		if(parser->commandNum != 0){
			int i;
			for(i = 0; i < parser->commandNum; i++){
				parseCommand(parser, parser->commands[i]);
			}
		}
	}
}

