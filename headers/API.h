#define _POSIX_C_SOURCE 2001112L
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <ctype.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/un.h>


int openConnection(const char *sockname, int msec, const struct timespec abstime);

int closeConnection(const char *sockname);

int openFile(const char *pathname, int flags);

int readNFiles(int N, const char *dirname);

int writeFile(const char, *pathname, const char *dirname);

int appendToFile(const char *pathname, void *buf, size_t size, const char *dirname);

int closeFile(const char *pathname);