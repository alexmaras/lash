#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#define MAXLENGTH 1000 

char *prompt = "\nLaSH % ";
static volatile int keepRunning = 1;

void intHandler(int dummy) {
    keepRunning = 0;
}

void printPrompt(){
    printf("%s", prompt);
}

int main(void){
    // define signal handler to prevent ctrl-c from killing the process
    signal(SIGINT, intHandler);
    char *input = (char*) malloc( MAXLENGTH );
    strcpy(input, "");
    size_t ln;
    
    while(strcmp("exit", input) != 0){
        printPrompt();
        fgets(input, MAXLENGTH, stdin);
        ln = strlen(input) - 1;
        if(input[ln] == '\n'){
            input[ln] = '\0';
        }
    }

    exit(0);
}
