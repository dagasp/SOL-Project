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

#include "hash.h"

#if !defined(BUFSIZE)
#define BUFSIZE 256
#endif

#if !defined(EXTRA_LEN_PRINT_ERROR)
#define EXTRA_LEN_PRINT_ERROR   512
#endif

#if !defined (SUCCESS)
#define SUCCESS 0 
#endif

#if !defined (FAILED)
#define FAILED -1 
#endif

#if !defined(MAXBACKLOG)
#define MAXBACKLOG   32
#endif

/*Header contenente funzioni utilità e strutture dati d'appoggio*/
typedef struct config_file {
    long memory_space;
    unsigned int num_of_threads;
    char sock_name[BUFSIZE];
    char directory[BUFSIZE];
} config_file;

typedef struct msg {
  char *data;
  size_t size;
} msg;

typedef struct file {
  unsigned int status;
  void *data;
} file;

enum STATUS {
  OPEN = 0, 
  CLOSED = 1
};

#define SYSCALL_EXIT(name, r, sc, str, ...)	\
    if ((r=sc) == -1) {				\
	perror(#name);				\
	int errno_copy = errno;			\
	print_error(str, __VA_ARGS__);		\
	exit(errno_copy);			\
    }

#define SYSCALL_PRINT(name, r, sc, str, ...)	\
    if ((r=sc) == -1) {				\
	perror(#name);				\
	int errno_copy = errno;			\
	print_error(str, __VA_ARGS__);		\
	errno = errno_copy;			\
    }

#define SYSCALL_RETURN(name, r, sc, str, ...)	\
    if ((r=sc) == -1) {				\
	perror(#name);				\
	int errno_copy = errno;			\
	print_error(str, __VA_ARGS__);		\
	errno = errno_copy;			\
	return r;                               \
    }

#define CHECK_EQ_EXIT(name, X, val, str, ...)	\
    if ((X)==val) {				\
        perror(#name);				\
	int errno_copy = errno;			\
	print_error(str, __VA_ARGS__);		\
	exit(errno_copy);			\
    }

#define CHECK_NEQ_EXIT(name, X, val, str, ...)	\
    if ((X)!=val) {				\
        perror(#name);				\
	int errno_copy = errno;			\
	print_error(str, __VA_ARGS__);		\
	exit(errno_copy);			\
    }

/**
 * \brief Procedura di utilita' per la stampa degli errori
 *
 */
static inline void print_error(const char * str, ...) {
    const char err[]="ERROR: ";
    va_list argp;
    char * p=(char *)malloc(strlen(str)+strlen(err)+EXTRA_LEN_PRINT_ERROR);
    if (!p) {
	perror("malloc");
        fprintf(stderr,"FATAL ERROR nella funzione 'print_error'\n");
        return;
    }
    strcpy(p,err);
    strcpy(p+strlen(err), str);
    va_start(argp, str);
    vfprintf(stderr, p, argp);
    va_end(argp);
    free(p);
}


/** 
 * \brief Controlla se la stringa passata come primo argomento e' un numero.
 * \return  0 ok  1 non e' un numbero   2 overflow/underflow
 */
static inline int isNumber(const char* s, long* n) {
  if (s==NULL) return 1;
  if (strlen(s)==0) return 1;
  char* e = NULL;
  errno=0;
  long val = strtol(s, &e, 10);
  if (errno == ERANGE) return 2;    // overflow/underflow
  if (e != NULL && *e == (char)0) {
    *n = val;
    return 0;   // successo 
  }
  return 1;   // non e' un numero
}

#define LOCK(l)      if (pthread_mutex_lock(l)!=0)        { \
    fprintf(stderr, "ERRORE FATALE lock\n");		    \
    pthread_exit((void*)EXIT_FAILURE);			    \
  }   
#define LOCK_RETURN(l, r)  if (pthread_mutex_lock(l)!=0)        {	\
    fprintf(stderr, "ERRORE FATALE lock\n");				\
    return r;								\
  }   

#define UNLOCK(l)    if (pthread_mutex_unlock(l)!=0)      {	    \
    fprintf(stderr, "ERRORE FATALE unlock\n");			    \
    pthread_exit((void*)EXIT_FAILURE);				    \
  }
#define UNLOCK_RETURN(l,r)    if (pthread_mutex_unlock(l)!=0)      {	\
    fprintf(stderr, "ERRORE FATALE unlock\n");				\
    return r;								\
  }
#define WAIT(c,l)    if (pthread_cond_wait(c,l)!=0)       {	    \
    fprintf(stderr, "ERRORE FATALE wait\n");			    \
    pthread_exit((void*)EXIT_FAILURE);				    \
  }
/* ATTENZIONE: t e' un tempo assoluto! */
#define TWAIT(c,l,t) {							\
    int r=0;								\
    if ((r=pthread_cond_timedwait(c,l,t))!=0 && r!=ETIMEDOUT) {		\
      fprintf(stderr, "ERRORE FATALE timed wait\n");			\
      pthread_exit((void*)EXIT_FAILURE);				\
    }									\
  }
#define SIGNAL(c)    if (pthread_cond_signal(c)!=0)       {		\
    fprintf(stderr, "ERRORE FATALE signal\n");				\
    pthread_exit((void*)EXIT_FAILURE);					\
  }
#define BCAST(c)     if (pthread_cond_broadcast(c)!=0)    {		\
    fprintf(stderr, "ERRORE FATALE broadcast\n");			\
    pthread_exit((void*)EXIT_FAILURE);					\
  }
static inline int TRYLOCK(pthread_mutex_t* l) {
  int r=0;		
  if ((r=pthread_mutex_trylock(l))!=0 && r!=EBUSY) {		    
    fprintf(stderr, "ERRORE FATALE unlock\n");		    
    pthread_exit((void*)EXIT_FAILURE);			    
  }								    
  return r;	
}

/*Legge dal file config.txt i dati*/

config_file *read_config(char *config); // da implementare

/** Evita letture parziali
 *
 *   \retval -1   errore (errno settato)
 *   \retval  0   se durante la lettura da fd leggo EOF
 *   \retval size se termina con successo
 */
static inline int readn(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char*)buf;
    while(left>0) {
	if ((r=read((int)fd ,bufptr,left)) == -1) {
	    if (errno == EINTR) continue;
	    return -1;
	}
	if (r == 0) return 0;   // EOF
        left    -= r;
	bufptr  += r;
    }
    return size;
}


/** Evita scritture parziali
 *
 *   \retval -1   errore (errno settato)
 *   \retval  0   se durante la scrittura la write ritorna 0
 *   \retval  1   se la scrittura termina con successo
 */
static inline int writen(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char*)buf;
    while(left>0) {
	if ((r=write((int)fd ,bufptr,left)) == -1) {
	    if (errno == EINTR) continue;
	    return -1;
	}
	if (r == 0) return 0;  
        left    -= r;
	bufptr  += r;
    }
    return 1;
}


#endif /* _UTIL_H */
