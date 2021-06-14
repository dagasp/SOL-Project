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


int main (int argc, char **argv) {

    //Maschere per i segnali 
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT); 
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGHUP);
    
    if (pthread_sigmask(SIG_BLOCK, &mask,NULL) != 0) {
	    fprintf(stderr, "FATAL ERROR\n");
	    goto _exit;
    }
}