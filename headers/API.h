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

enum OP_CODE {
    OPENFILE = 0,
    READFILE = 1,
    READNFILES = 2,
    WRITEFILE = 3,
    APPENDTOFILE = 4,
    CLOSEFILE = 5,
    CLOSECONNECTION = 6
};

typedef struct client_operations {
    char pathname[MAX_PATH];
    char data[MAX_FILE_SIZE];
    char dirname [MAX_PATH];
    long client_desc;
    unsigned int op_code;
    int flags;
    int feedback;
    int files_to_read;
    size_t size;
} client_operations;

typedef struct server_reply {
    char pathname[MAX_PATH];
    char data[MAX_FILE_SIZE];
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

int writeToFile(char *pathname, char *content, const char *dirname);

#endif