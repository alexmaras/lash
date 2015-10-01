lashmake: lash.c
	gcc main.c lash.c lashparser.c -o bin/lash -Wall -lreadline
