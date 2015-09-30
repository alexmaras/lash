bool isEscaped(char *line, int index);
bool followedBySemiColon(char *line, int index);
int insideQuotes(int index, int quotes[][3], int numberOfQuotePairs);
bool atStart(int index, char *line);
bool atEnd(int index, char *line);
void stripStartAndEndSpacesAndSemiColons(char *line);
void cleanString(char *line);
void removeEscapeSlashesAndQuotes(char *line);
int parseCommand(char *command, char **args);
int splitCommands(char *line, char **commands);
bool indexNotInArray(int array[][3], int arrayIndex, int foundCharIndex);
int findQuoteLocations(char *line, int quotes[][3]);
void executeCommand(char **args);
void emptyArray(char **array, int length);
void sighandler(int signum);

