#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>

#include "shell.h"

/*
Fonction permettant de savoir si l'allocation de la mémoire a bien abouti
*/
void check_failure(void *ptr) {
    if (ptr == NULL) {
        fprintf(stderr, "Allocation de la mémoire échouée\n");
        exit(EXIT_FAILURE);
    }
}

/*
Fonction permettant de récupérer une commande avec les options et arguments
*/
char **split(char *raw_cmd, char *limit) {
    char *ptr = NULL;      //pointeur pour stocker les tokens extraits de raw_cmd.
    char **cmd = NULL;     //tableau de pointeurs pour stocker les tokens
    short int i = 0;          //compteur pour suivre le nombre de tokens.

    //strtok pour extraire la commande avec les options ou arguments
    ptr = strtok(raw_cmd, limit);

    //boucle pour extraire les tokens
    while (ptr) {
        //reallocation de la mémoire pour chaque nouveau token
        cmd = (char **)realloc(cmd, ((i + 1) * sizeof(char *)));
        check_failure(cmd);
        //stockage du token dans cmd
        cmd[i] = strdup(ptr);
        //passage au token suivant
        ptr = strtok(NULL, limit);
        ++i;
    }
    //on realloue la mémoire afin d'insérer NULL pour marquer la fin du tableau
    cmd = (char **)realloc(cmd, ((i + 1) * sizeof(char *)));
    check_failure(cmd);
    cmd[i] = NULL;
    //retourner le tableau
    return cmd;
}

/*
Fonction permettant de libérer de la memoire dans un tableau
*/
void free_array(char **array) {
	if (array == NULL) //si le tableau est vide, on sort de la fonction
        return;
    for (int i = 0; array[i]; i++) { //ici on libere la mémoire pour chaque élément du tableau
        free(array[i]);
    }
    free(array);
}

/*
Fonction permettant d'effectuer une redirection >
*/
void redirection_1(char **c) {
        char buffer[256];
        ssize_t bytes_read;
        
        /*Utiliser open pour ouvrir le fichier source en lecture seule. Si le fichier source n'existe pas, le programme
        doit afficher un message d'erreur et se terminer.*/
        int fd_source = open(c[0],O_RDONLY); 

        if (fd_source == -1){
            printf("Erreur fichier source: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        /* Utiliser à nouveau open pour ouvrir le fichier destination en écriture seulement. Si le fichier destination
        n'existe pas, il doit être créé avec les permissions rw-r--r--. Si le fichier destination existe déjà, il doit être
        remis à zéro. */
        int fd_dest = open(c[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_dest == -1){
            printf("Erreur fichier destination: %s\n", strerror(errno));
            close(fd_source);
            exit(EXIT_FAILURE);
        }
        /* Utiliser une boucle avec read pour lire du contenu du fichier source dans un buffer, puis write pour
        écrire ce contenu dans le fichier destination. Cette boucle doit se poursuivre jusqu'à ce que tout le
        contenu du fichier source ait été copié. */
        while ((bytes_read = read(fd_source, buffer, sizeof(buffer))) > 0) {
            ssize_t bytes_write = write(fd_dest, buffer, bytes_read);
            if (bytes_write != bytes_read) {
                printf("Erreur lors de l'écriture sur la sortie standard : %s\n", strerror(errno));
                close(fd_dest);
                close(fd_source);
                exit(EXIT_FAILURE);
            }
        }
        /*Une fois la copie terminée, utiliser close pour fermer les descripteurs de fichiers des fichiers source et
        destination.*/
        close(fd_dest);
        close(fd_source);
}

/*
Fonction permettant d'effectuer une redirection <
*/
void redirection_2(char **c) {
        char buffer[256];
        ssize_t bytes_read;
        
        /*Utiliser open pour ouvrir le fichier source en lecture seule. Si le fichier source n'existe pas, le programme
        doit afficher un message d'erreur et se terminer.*/
        int fd_source = open(c[2],O_RDONLY); 

        if (fd_source == -1){
            printf("Erreur fichier source: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        /* Utiliser à nouveau open pour ouvrir le fichier destination en écriture seulement. Si le fichier destination
        n'existe pas, il doit être créé avec les permissions rw-r--r--. Si le fichier destination existe déjà, il doit être
        remis à zéro. */
        int fd_dest = open(c[0], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_dest == -1){
            printf("Erreur fichier destination: %s\n", strerror(errno));
            close(fd_source);
            exit(EXIT_FAILURE);
        }
        /* Utiliser une boucle avec read pour lire du contenu du fichier source dans un buffer, puis write pour
        écrire ce contenu dans le fichier destination. Cette boucle doit se poursuivre jusqu'à ce que tout le
        contenu du fichier source ait été copié. */
        while ((bytes_read = read(fd_source, buffer, sizeof(buffer))) > 0) {
            ssize_t bytes_write = write(fd_dest, buffer, bytes_read);
            if (bytes_write != bytes_read) {
                printf("Erreur lors de l'écriture sur la sortie standard : %s\n", strerror(errno));
                close(fd_dest);
                close(fd_source);
                exit(EXIT_FAILURE);
            }
        }
        /*Une fois la copie terminée, utiliser close pour fermer les descripteurs de fichiers des fichiers source et
        destination.*/
        close(fd_dest);
        close(fd_source);
}

/*
Fonction permettant d'executer les commandes de l'utilisateur
*/
void execute(char **c) {
    
    if (strcmp(c[0], "cd") == 0) {
        if (c[1] != NULL) { //on vérifie si le deuxieme argument est vide
            if (chdir(c[1]) != 0) { //on vérifie si le répertoire existe
                perror("cd");
            }
        } 
        else {
            fprintf(stderr, "cd: argument manquant\n");
        }
    }
    //Redirection '>'
    else if (c[1] != NULL && strcmp(c[1], ">") == 0) 
        redirection_1(c);
    else if (c[1] != NULL && strcmp(c[1], "<") == 0)
        redirection_2(c);
    else {
        int fd[2]; //file descriptor
        int r = pipe(fd);
        pid_t pid = fork();
        
        if (r < 0) {
            perror("Erreur Pipe");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) { //processus enfant
            close(fd[0]);
            //tout ce qui est écrit sera dans le tube
            dup2(fd[1], STDOUT_FILENO);
            close(fd[1]);
            execvp(c[0], c); //execvp car pas besoin de connaitre le chemin du programme
        } 
        else { //processus parent, son role est de lire les données envoyées par le processus enfant
            close(fd[1]);
            char buffer[1024]; //permet de stocker les données à afficher temporairements
            ssize_t bytes;
            while ((bytes = read(fd[0], buffer, sizeof(buffer) - 1)) > 0) {
                buffer[bytes] = '\0'; //permet de savoir si on est bien à la fin de la chaine de caractere
                write(STDOUT_FILENO, buffer, bytes); //permet d'afficher dans le terminal le resultat de la commande
            }
        }
    }
}

void shell(char *buffer,size_t buf_size, char **cmd){
    buffer = (char *)calloc(sizeof(char), buf_size); //buffer pour stocker la commande de l'utilisateur initialisé à 0
    check_failure(buffer);

    //ecriture d'un prompt
    write(1, "csh> ", 5);

    //lecture de l'entrée standard
    while (getline(&buffer, &buf_size, stdin) > 0) {
        cmd = split(buffer, " \n\t"); //on recupere la commande et on utilise les espaces, les sauts de lignes et tabulations
        execute(cmd);
        write(1, "csh> ", 5);
        free_array(cmd);
    }
    free(buffer);
}