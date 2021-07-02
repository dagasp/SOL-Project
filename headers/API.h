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

#include "util.h"

#define UNIX_PATH_MAX 108 
#define O_CREATE 0
#define O_LOCK 1

enum OP_CODE {
    OPENFILE = 0,
    READFILE = 1,
    READNFILES = 2,
    WRITEFILE = 3,
    APPENDTOFILE = 4
};

typedef struct client_operations {
    char pathname[BUFSIZE];
    char data[BUFSIZE];
    unsigned int op_code;
    int flags;
    int feedback;
    int files_to_read;
} client_operations;

typedef struct server_reply {
    char pathname[BUFSIZE];
    char data[BUFSIZE];
    unsigned int size;
    unsigned int reply_code;
    int n_files_letti;
} server_reply;

/*API d'appoggio al Client*/

int openConnection(const char *sockname, int msec, const struct timespec abstime);

int closeConnection(const char *sockname);

int openFile(const char *pathname, int flags);

int readFile(const char* pathname, void** buf, size_t* size);

int readNFiles(int N, const char *dirname);

int writeFile(const char *pathname, const char *dirname);

int appendToFile(const char *pathname, void *buf, size_t size, const char *dirname);

int closeFile(const char *pathname); 

#endif