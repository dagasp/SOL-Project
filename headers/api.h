#if !defined(_API_H)
#define _API_H


//#define _POSIX_C_SOURCE 2001112L
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <ctype.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <limits.h>
#include <libgen.h>

#include "util.h"

#define UNIX_MAX_PATH 108 
#define DIR_SIZE 20
#define O_CREATE 0
#define O_LOCK 1
#define O_CREATE_OR_O_LOCK 2



/*API d'appoggio al Client*/

int writeToFile(char *pathname, char *content, const char *dirname);

int openConnection(const char *sockname, int msec, const struct timespec abstime);

int closeConnection(const char *sockname);

int openFile(const char *pathname, int flags);

int readFile(const char* pathname, void** buf, size_t* size);

int readNFiles(int N, const char *dirname);

//int writeFile(const char *pathname, const char *dirname);

int appendToFile(const char *pathname, void *buf, size_t size, const char *dirname);

int closeFile(const char *pathname);

int writeFile (char *file_name);


#endif
