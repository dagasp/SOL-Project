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


//MyHeaders
#include "util.h"
#include "hash.h"
#include "threadpool.h"

static config_file *conf;

int main (int argc, char **argv) {
    if ((conf = read_config("./test/config.txt")) == NULL) {
        fprintf(stderr, "Errore nella lettura del file config.txt\n");
        exit(EXIT_FAILURE);
    }
    int num_of_threads_in_pool;
    //Maschere per i segnali 
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT); 
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGHUP);
    
    if (pthread_sigmask(SIG_BLOCK, &mask,NULL) != 0) {
	    fprintf(stderr, "FATAL ERROR\n");
        exit(EXIT_FAILURE);
    }
}