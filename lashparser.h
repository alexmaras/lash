#ifndef LASHPARSER_H_
#define LASHPARSER_H_

#include <stdbool.h>

#define PIPE 1
#define REDIRECTBACKWARD 2
#define REDIRECTFORWARD 3

struct LashParser;

struct LashParser *newLashParser();

bool atEnd(int index, char *line);
bool atStart(int index, char *line);
void cleanString(struct LashParser *parser, char *line);
int  copyString(char *copyTo, char *copyFrom, int startAt, int endAt);
int  findQuoteLocations(char *line, int quotes[][3]);
bool followedBySemiColon(char *line, int index);
bool indexNotInArray(int array[][3], int arrayIndex, int foundCharIndex);
int  insideQuotes(int index, int quotes[][3], int numberOfQuotePairs);
bool isEscaped(char *line, int index);
int  parseCommand(struct LashParser *parser, char *command, char **args, int *pipeOrRedirect);
void removeEscapeSlashesAndQuotes(struct LashParser *parser, char *line);
void sighandler(int signum);
int  splitCommands(struct LashParser *parser, char *line, char **commands);
void stripStartAndEndSpacesAndSemiColons(char *line);
int  findPipes(struct LashParser *parser, char *line, int *pipeIndexes);
int  findRedirects(struct LashParser *parser, char *line, int redirectIndexes[][2]);
int foundPipeOrRedirect(int index, int *pipes, int pipenum, int redirects[][2], int redirectnum);
#endif
