#ifndef SHELL_H 
#define SHELL_H

#include <stdlib.h>

//=============================================================================
// Prototypes
//=============================================================================

void check_failure(void *ptr);
char **split(char *raw_cmd, char *limit);
void free_array(char **array);
void execute(char **c);
void shell(char *buffer,size_t buf_size, char **cmd);

#endif