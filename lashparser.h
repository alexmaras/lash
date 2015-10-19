#ifndef LASHPARSER_H_
#define LASHPARSER_H_

#define QUOTE_MISMATCH -1
#define VALID 1
#define NO_ARGS 0

struct Command{
	char *command;
	char **args;
	char symbolAfter;
	char *redirectIn;
	char *redirectOut;
	int argNum;
};

struct LashParser {
	int maxcommands;
	int maxargs;
	int maxarglength;
	int maxinput;
	struct Command **commands;
	int commandNum;
};

// struct construction and destruction functions
struct Command    *newCommand(struct LashParser *parser);
struct LashParser *newLashParser(int commands, int args, int arglength);
void   clearParser(struct LashParser *parser);

// command building and parsing functions
int buildCommand (struct LashParser *parser, char *line);
int splitCommands(struct LashParser *parser, char *line);
int parseCommand (struct LashParser *parser, struct Command *commData, int commandIndex);

// string check functions
int atEnd(const int index, const char *line);
int atStart(const int index, const char *line);
int isEscaped(const char *line, const int index);
int insideQuotes(const int index, int quotes[][3], const int numberOfQuotePairs);
int indexNotInArray(int array[][3], const int arrayIndex, const int foundCharIndex);
int findQuoteLocations(const char *line, int quotes[][3]);
int followedBySemiColonOrAmpersand(const char *line, const int index);

// string cleanup/manipulation functions
int  copyString(char *copyTo, const char *copyFrom, int startAt, const int endAt);
void cleanString(struct LashParser *parser, char *line);
void replaceTilde(struct LashParser *parser, char *line);
void removeEscapeSlashesAndQuotes(struct LashParser *parser, char *line);
void stripStartAndEndSpacesAndEndingSymbols(char *line);

#endif
