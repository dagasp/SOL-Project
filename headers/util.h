#if !defined(_UTIL_H)
#define _UTIL_H

#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>

#if !defined(BUF_DIM)
#define BUF_DIM 256
#endif

/*Header contenente funzioni utilità e strutture dati d'appoggio*/
typedef struct config_file {
    int memory_space;
    unsigned int num_of_threads;
    char sock_name[BUF_DIM];
} config_file;

config_file *read_config(char *config) {} //Funzione che legge i dati dal file config.txt - da implementare




#endif