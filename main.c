#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>

#include "shell.h"

int	main(int argc, char *argv[]) {

    /*
    Il faut 1 argument obligatoire pour lancer le programme
    -c : pour lancer le programme avec une commande
    -s : pour start le programme
    */
    if (argc < 2) { 
        printf("Usage: %s doit contenir 1 argument\n-c <cmd> : pour lancer une commande. Avec options : ls/-la\n-s : pour lancer le shell\n", argv[0]);
        return 1;
    }

    char *buffer = NULL;
	char **cmd = NULL;
    size_t buf_size = 1024;
    
    /*
    Les conditions lorsqu'on rencontre les options
    */
    if(strcmp(argv[1],"-c")==0){
        cmd = split(argv[2]," /\n\t");
        execute(cmd);
        shell(buffer,buf_size,cmd);
    }
    else if (strcmp(argv[1],"-s")==0) {
        shell(buffer,buf_size,cmd);
    }

	free(buffer);
    return 0;
}