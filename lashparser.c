#include <stdlib.h>
#include <string.h>
#include <glob.h>

#include "lashparser.h"




//------------------------------------------------------------------------------
// struct construction and destruction functions
//------------------------------------------------------------------------------
/**
 * Constructs a new Command struct using the given parser for command length
 * and maximum arguments. The Command struct returned has the redirectIn
 * and redirectOut variables initialised to NULL, the argNum variable set to 0
 * and the symbolAfter variable set to a space (' ')
 *
 * @param  parser A preconstucted LashParser struct with appropriately set max values
 * @return        A constructed Command struct
 */
struct Command *newCommand(struct LashParser *parser){

	struct Command *newCommand = (struct Command *)malloc(sizeof(struct Command));
	newCommand->args = malloc(sizeof(char) * parser->maxargs);

	newCommand->symbolAfter = ' ';
	newCommand->argNum = 0;
	newCommand->redirectIn = NULL;
	newCommand->redirectOut = NULL;
	return newCommand;
}

/**
 * Constructs a new LashParser struct given the provided variables.
 * The commands parameter sets the maximum amount of commands the struct can
 * store, the args parameter sets the maximum number of args per commands and
 * the arglength parameter sets the maximum string length of each argument
 *
 * @param  commands  maximum # of commands
 * @param  args      maximum # of args/command
 * @param  arglength maximum string length of each arg
 * @return           A fully constructed LashParser struct
 */
struct LashParser *newLashParser(int commands, int args, int arglength){
	struct LashParser *parser = (struct LashParser *)malloc(sizeof(struct LashParser));
	parser->maxcommands = commands;
	parser->maxargs = args;
	parser->maxarglength = arglength;
	parser->maxinput = commands * args * arglength;

	parser->commands = malloc(sizeof(struct Command) * commands);

	return parser;
}

/**
 * Frees any memory used by the LashParser struct AND the commands stored
 * within the LashParser.
 * IMPORTANT: WHEN THIS IS RUN, IT WILL FREE ALL COMMANDS CREATED USING THIS
 * LASHPARSER STRUCT
 *
 * @param parser A fully constructed LashParser struct
 */
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
	parser->commandNum = 0;

}



//------------------------------------------------------------------------------
// command building and parsing functions
//------------------------------------------------------------------------------
/**
 * Performs all necessary functions in order to parse and build a full LashParser
 * struct with commands and arguments given a string input.
 *
 * @param  parser A fully constructed LashParser struct
 * @param  line   A raw string under the max string length defined as maxcommands * maxargs * arglength
 * @return        In integer referencing the oucome of the build. VALID, NO_ARGS or QUOTE_MISMATCH are possible
 * no args referring to if the function was sent an empty string, VALID meaning the command was
 * successfully parsed (NOT whether or not the command itself is valid) and QUOTE_MISMATCH meaning
 * the string sent contained an odd number of non-escaped quotes.
 */
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


/**
 * Splits the command at its fundamental level into individual "commands". Note that
 * this stage alone is not enough as it will not split each command into its respective
 * arguments, just each command will be set within the commands[] array.
 *
 * @param  parser A fully constructed LashParser struct
 * @param  line   A string with no mismatched quotes
 */
void splitCommands(struct LashParser *parser, char *line){

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
}

/**
 * Parses a given command (split previously by splitCommands()) for arguments, expands
 * wildcards and tiles, strips extraneous spaces and irrelevant characters
 *
 * @param parser       A fully constructed LashParser struct
 * @param commData     A fully constructed Command struct
 * @param commandIndex The index of the command in the LashParser struct
 */
void parseCommand(struct LashParser *parser, struct Command *commData, int commandIndex){
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
}


//------------------------------------------------------------------------------
// string check functions
//------------------------------------------------------------------------------
/**
 * Check whether an index is at the end of the string barring any semi-colons or
 * spaces. If there is nothing after the index in the line, or only spaces or
 * semi-colons, then this function will return 1. Otherwise, it will return 0
 *
 * @param  index The index in the line to check (this is the character that should be at the end)
 * @param  line  The string that the index is contained within
 * @return       1 if at the end, 0 if not
 */
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


/**
 * Check whether an index is at the front of the string barring any semi-colons or
 * spaces. If there is nothing before the index in the line, or only spaces or
 * semi-colons, then this function will return 1. Otherwise, it will return 0
 *
 * @param  index The index in the line to check (this is the character that should be at the start)
 * @param  line  The string that the index is contained within
 * @return       1 if at the end, 0 if not
 */
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


/**
 * Checks whether the character at the index given is escaped. This is a recursive
 * function and as such will backcheck for multiple characters to determine if it
 * really escaped.
 *
 * @param  line  The string to check within
 * @param  index The index of the character to check for escaping
 * @return       1 if the character is escaped, 0 otherwise
 */
int isEscaped(const char *line, const int index){
    if(index == 0)
        return 0;
    if(line[index-1] == '\\' && !isEscaped(line, index-1)){
        return 1;
    }
    return 0;
}

/**
 * Checks whether the given index is within quotes. This function takes an array
 * of quotes and the number of quotes from the findQuoteLocations function (which
 * should be run before calling insideQuotes) which, as a result, will be filled
 * with the positions of the quotes inside a string. This function purely checks
 * whether the index smaller than the upper and larger than the lower indexes
 * of one of the pairs of quotes listed in the quotes array
 *
 * @param  index              The index to check for containment within quotes
 * @param  quotes[][3]        The quotes array generated by findQuoteLocations which lists the locations of the quotes in a given string
 * @param  numberOfQuotePairs The length of the quotes array (returned by the findQuoteLocations function)
 * @return                    Returns 0 for not inside quotes or an int representing the quote type (1 for single quotes, 2 for double)
 */
int insideQuotes(const int index, int quotes[][3], const int numberOfQuotePairs){
    int i;
    for(i = 0; i < numberOfQuotePairs; i++){
        if(quotes[i][0] < index && index < quotes[i][1]){
            return quotes[i][2];
        }
    }
    return 0;
}

/**
 * Checks whether a given indes is NOT listed in an array. This function is used
 * purely by the findQuoteLocations() function to avoid code duplication and is very
 * specific to its uses, requiring an array of size [variables] by 3. The arrayIndex refers
 * to the current size of the array while the foundCharIndex refers to the position
 * of a character in a particulat string
 *
 * @param  array[][3]     The incomplete quotes array
 * @param  arrayIndex     The length of the array so far
 * @param  foundCharIndex The index to search within the array for
 * @return                1 for not within array, 0 if the index was within the array
 */
int indexNotInArray(int array[][3], const int arrayIndex, const int foundCharIndex){
    int index;
    for(index = 0; index < arrayIndex; index++){
        if(array[index][0] == foundCharIndex || array[index][1] == foundCharIndex){
            return 0;
        }
    }
    return 1;
}


/**
 * Finds the locations of quotes in a given string. The array passed to this
 * function will be altered and is of the following format:
 * quotes[indexOfQuotePair][0] = index of the opening quote
 * quotes[indexOfQuotePair][1] = index of the closing quote
 * quotes[indexOfQuotePair][2] = Integer representing the type of quote: 1 == single quote, 2 == double quote
 *
 * @param  An empty integer array of a given length with 3 elements per element
 * @param  A string to search for quotes within
 * @return The number of pairs of quotes found
 */
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

/**
 * Checks whether the index given in the given string if followed by a semi-colon
 * or an ampersand. This ignores spaces when searching ahead and so a case such as
 * followedBySemiColonOrAmpersand("fishing    ;", 6) will return true while
 * followedBySemiColonOrAmpersand("fishing    \;", 6) will return false
 *
 * @param  line  The string to check within
 * @param  index The index to check for a following ampersand or semicolon
 * @return       1 if followed by a semicolon or ampersand, 0 if not
 */
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


//------------------------------------------------------------------------------
// string cleanup/manipulation functions
//------------------------------------------------------------------------------
/**
 * Copies the given string (copyFrom) to another given string (copyTo) from a
 * starting index (startAt) to an ending index (endAt). This function does NOT
 * include the index given at endAt and will insert a null character after the final
 * copied index. Note that startAt will be altered when using this function.
 *
 * As an example, the call
 * copyString(newString, "hello world", 3, 7) will change newString to "lo w" and will
 * return 8
 *
 * @param  copyTo   An empty string to copy characters into
 * @param  copyFrom A non-empty string to copy characters from
 * @param  startAt  A starting index to begin from
 * @param  endAt    An ending index
 * @return          Returns the index of the character after the finished value of startAt
 */
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

/**
 * "Cleans" the string according to the needs of the LashParser. This means that
 * the given line will be stripped of leading or trailing spaces and semi-colons
 * as well as semi-colons followed by semi-colons (unecessary duplication) and
 * spaces followed by spaces or spaces followed by or preceding redirect or pipe
 * symbols UNLESS they are escaped or in quotes.
 * This function helps to clean up the string of erenous characters in order
 * to be able to properly parse it without unecessary care taken to deal with irrelevant
 * characters.
 * This function would likely not be useful outside of this application as it
 * prepares a string for being parsed by this specific parser.
 *
 * @param parser A fully constructed LashParser struct
 * @param line   The string to "clean"
 */
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
                        (atStart(i, line)) ||   // semicolon at start
                        (atEnd(i, line))   ||   // semicolon at end
                        (line[i+1] == ';') ||   // semicolon followed by semicolon
                        (j == 0)                // first character of the new string
                    )
                )
                &&
                !(line[i] == ' ' &&
                    (
                        (atStart(i, line)) ||   // space at start
                        (atEnd(i, line))   ||   // space at end
                        (line[i+1] == ' ') ||   // space followed by space
                        (line[i+1] == '|') ||
                        (line[i+1] == '<') ||
                        (line[i+1] == '>') ||
                        (line[i-1] == '|' && !isEscaped(line, i-1)) ||
                        (line[i-1] == '<' && !isEscaped(line, i-1)) ||
                        (line[i-1] == '>' && !isEscaped(line, i-1)) ||
                        (j == 0)                // first character of the new string
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

/**
 * Expands a tilde to be the users home directory if it is the first character in a string
 * If the tilde is not the first character in the string, it will NOT be expanded
 *
 * @param parser A fully constructed LashParser struct
 * @param line   The string to have the tilde expanded
 */
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

/**
 * Builds a glob struct with expanded tokens from the given string
 * If there are no wildcard characters, thie functions returned glob_t
 * struct will have gl_pathc set to 0.
 *
 * @param  parser A fully constructed LashPatser struct
 * @param  line   A string to check for wildcards to be expanded
 * @return        [description]
 */
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

/**
 * Removes backslashes used to escape characters and any quotation marks which were
 * not escaped themselves from the given string. This is used to cleanse a string
 * before it is split and stored as a list of arguments. Once this step is complete,
 * if there string were echoed, it would look exactly as it does after the function finishes.
 *
 * @param parser A fully constructed LashParser struct
 * @param line   A string from which to remove quotes and escape slashes
 */
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

/**
 * Removes uneccesary characters at the start and end of the given string.
 * This includes semi-colons, ampersands and spaces (as long as they haven't been
 * escaped). This function should be run after storing the trailing character
 * in order to reliably parse whether the job should be run in the background
 * or in the foreground.
 *
 * @param line The line from which the trailing spaces, semi-colons and ampersands should be removed.
 */
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
