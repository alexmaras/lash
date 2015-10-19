#include <stdlib.h>
#include <string.h>
#include <glob.h>

//#include <stdio.h>

#include "lashparser.h"

struct Command *newCommand(struct LashParser *parser){

	struct Command *newCommand = (struct Command *)malloc(sizeof(struct Command));
	newCommand->args = malloc(sizeof(char) * parser->maxargs);

	newCommand->symbolAfter = ' ';
	newCommand->argNum = 0;
	newCommand->redirectIn = NULL;
	newCommand->redirectOut = NULL;
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
		free(parser->commands[i]->command);
		free(parser->commands[i]->args);
		if(parser->commands[i]->redirectIn != NULL){
			free(parser->commands[i]->redirectIn);
		}
		if(parser->commands[i]->redirectOut != NULL){
			free(parser->commands[i]->redirectOut);
		}
		free(parser->commands[i]);
	}

}

int isEscaped(const char *line, const int index){
	if(index == 0)
		return 0;
    if(line[index-1] == '\\' && !isEscaped(line, index-1)){
        return 1;
    }
    return 0;
}

int followedBySemiColonOrAmpersand(const char *line, const int index){
    int i = index+1;
    while(line[i] == ' '){
        i++;
    }
    if(line[i] == ';' || line[i] == '&'){
        return 1;
    }

    return 0;
}

int insideQuotes(const int index, int quotes[][3], const int numberOfQuotePairs){
    int i;
    for(i = 0; i < numberOfQuotePairs; i++){
        if(quotes[i][0] < index && index < quotes[i][1]){
            return quotes[i][2];
        }
    }
    return 0;
}

int atStart(const int index, const char *line){
    int ending = 0;
    int i;

    for(i = index-1; i >= ending; i--){
        if(line[i] != ' ' && line[i] != ';'){
            return 0;
        }
    }
    return 1;
}


int atEnd(const int index, const char *line){
    int ending = strlen(line);
    int i;

    for(i = index+1; i < ending; i++){
        if(line[i] != ' ' && line[i] != ';'){
            return 0;
        }
    }
    return 1;
}



void stripStartAndEndSpacesAndEndingSymbols(char *line){
	int length = strlen(line);
	int j = 0;
	int i = 0;

	char *newline = (char*) malloc( length+1 );

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
			&&
			!(line[i] == '&' &&
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
			isEscaped(line, i) ||
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
						(line[i-1] == '|' && !isEscaped(line, i-1)) ||
						(line[i-1] == '<' && !isEscaped(line, i-1)) ||
						(line[i-1] == '>' && !isEscaped(line, i-1)) ||
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
	int enclosingQuote;

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
		enclosingQuote = 0;
		for(k = 0; k < numberOfQuotePairs; k++){
			if(quotes[k][0] == i || quotes[k][1] == i)
				enclosingQuote = 1;
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
		int copyCharacter = (
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

int copyString(char *copyTo, const char *copyFrom, int startAt, const int endAt){
	int i = 0;
	while(startAt < endAt){
		copyTo[i] = copyFrom[startAt];
		i++;
		startAt++;
	}
	copyTo[i] = '\0';
	return startAt+1;
}

void replaceTilde(struct LashParser *parser, char *line){
	if(line[0] == '~'){
		char *tempString = malloc(sizeof(char) * parser->maxinput);
		char *buffer = malloc(sizeof(char) * parser->maxinput);

		strcpy(tempString, getenv("HOME"));
		memcpy(buffer, &(line[1]), strlen(line)-1);
		strcat(tempString, buffer);
		strcpy(line, tempString);

		free(buffer);
		free(tempString);
	}
}

glob_t * expandWildcards(struct LashParser *parser, char *line){
	int quotes[parser->maxcommands][3];
	int numberOfQuotePairs = findQuoteLocations(line, quotes);

	int length = strlen(line);
	int i;
	int runglob = 0;
	for(i = 0; i < length; i++){
		if((line[i] == '*' || line[i] == '?') && !isEscaped(line, i) && !insideQuotes(i, quotes, numberOfQuotePairs)){
			runglob = 1;
		}
	}

	glob_t *globbuf = (glob_t*)malloc(sizeof(glob_t));
	if(runglob){
		glob(line, 0, NULL, globbuf);
	} else {
		globbuf->gl_pathc = 0;
	}
	return globbuf;
}

int parseCommand(struct LashParser *parser, struct Command *commData, int commandIndex){
	char *command = commData->command;
	// Get the quote locations for the current command
	int quotes[parser->maxcommands][3];
	int numberOfQuotePairs = findQuoteLocations(command, quotes);


    int i;
	int argnum = 0;
    int copyIndex = 0;
	int captureRedirect = 0;
    char currentChar;
    for(i = 0; i < strlen(command); i++){
		currentChar = command[i];
        int lastChar = (i == strlen(command)-1);

		// if this is the last character OR:
		//     the current character is a space that is NOT escaped AND is NOT in quotes
		// We have found a break and should copy that section into tempString
		int foundBreak = (
			lastChar ||
			(
				(currentChar == ' ' || currentChar == '|' || currentChar == '<' || currentChar == '>') &&
				!isEscaped(command, i) &&
				!insideQuotes(i, quotes, numberOfQuotePairs)
			)
		);
		if(foundBreak){
			char *tempString = (char*) malloc( (int)(parser->maxarglength * sizeof(char)));

			copyIndex = copyString(tempString, command, copyIndex, ( lastChar ? i+1 : i));
			stripStartAndEndSpacesAndEndingSymbols(tempString);
			replaceTilde(parser, tempString);
			glob_t *globbed = expandWildcards(parser, tempString);
			removeEscapeSlashesAndQuotes(parser, tempString);

			if(captureRedirect == 1){
				if(commData->redirectIn != NULL)
					free(commData->redirectIn);
				commData->redirectIn = tempString;
			} else if(captureRedirect == 2){
				if(commData->redirectOut != NULL)
					free(commData->redirectOut);
				commData->redirectOut = tempString;
			} else if(strlen(tempString) > 0){
				if(globbed->gl_pathc == 0){
					commData->args[argnum] = tempString;
					argnum++;
					commData->argNum = argnum;
					commData->args[argnum] = NULL; // Make sure final arg is always NULL so that execvp handles it correctly
					free(globbed);
				} else {
					int j;
					free(tempString);
					for(j = 0; j < globbed->gl_pathc; j++){
						commData->args[argnum] = (char*) malloc( (int)(parser->maxarglength * sizeof(char)));
						strcpy(commData->args[argnum],  globbed->gl_pathv[j]);
						argnum++;
						commData->argNum = argnum;
					}
					commData->args[argnum] = NULL;
					globfree(globbed);
				}
			}
			switch(currentChar){
				case '<':
					captureRedirect = 1;
					break;
				case '>':
					captureRedirect = 2;
					break;
				default:
					captureRedirect = 0;
					break;
			}
        }
    }
	return argnum;
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
        int lastChar = (i == strlen(line)-1);
        currentChar = line[i];
        if(lastChar || ((currentChar == '&' || currentChar == ';' || currentChar == '|') && !followedBySemiColonOrAmpersand(line, i) && !isEscaped(line, i) && !insideQuotes(i, quotes, numberOfQuotePairs))){
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
			parser->commands[commandnum]->symbolAfter = (lastChar && currentChar != '&' ? ';' : currentChar);
            commandnum++;
            copyIndex++;
        }
    }
	parser->commandNum = commandnum;
	return commandnum;
}
int indexNotInArray(int array[][3], const int arrayIndex, const int foundCharIndex){
    int index;
    for(index = 0; index < arrayIndex; index++){
        if(array[index][0] == foundCharIndex || array[index][1] == foundCharIndex){
            return 0;
        }
    }
    return 1;
}

int findQuoteLocations(const char *line, int quotes[][3]){
    int index, secondIndex, quoteIndex = 0;
    char currentChar = ' ';
    int linelength = strlen(line);
    int foundMatch = 0;

    for(index = 0; index < linelength; index++){
        currentChar = line[index];

        if((currentChar == '\"' || currentChar == '\'') && indexNotInArray(quotes, quoteIndex, index) && !isEscaped(line, index)){
            for(secondIndex = index+1; secondIndex < linelength; secondIndex++){
                foundMatch = 0;
                if(line[secondIndex] == currentChar && indexNotInArray(quotes, quoteIndex, index) && !isEscaped(line, secondIndex)){
                    foundMatch = 1;
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


int buildCommand(struct LashParser *parser, char *line){
	int quotes[parser->maxcommands][3];
	int numberOfQuotes = findQuoteLocations(line, quotes);
	if(numberOfQuotes == -1){
		return QUOTE_MISMATCH;
	} else {
		splitCommands(parser, line);
		if(parser->commandNum != 0){
			int i;
			for(i = 0; i < parser->commandNum; i++){
				parseCommand(parser, parser->commands[i], i);

			}
		} else {
			return NO_ARGS;
		}
	}
	return VALID;
}

