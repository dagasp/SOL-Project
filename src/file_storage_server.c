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
#include "api.h"

static config_file *conf;
static wargs *w_args;

void threadWorker(void *arg) {
    assert(arg);
    w_args = arg;
    long connFd = w_args->sock_fd;
    int r;
    server_reply *server_rep;
    client_operations *client_op;
    SYSCALL_RETURN("readn", r, readn(connFd, client_op, sizeof(server_rep)), "Impossibile leggere dati dal client\n", "");
    int op_code = client_op->op_code;
    switch (op_code) {
        case OPENFILE:
        /*Richiesta di apertura o di creazione di un file. La semantica della openFile dipende dai flags passati come secondo
argomento che possono essere O_CREATE ed O_LOCK. Se viene passato il flag O_CREATE ed il file esiste già
memorizzato nel server, oppure il file non esiste ed il flag O_CREATE non è stato specificato, viene ritornato un
errore. In caso di successo, il file viene sempre aperto in lettura e scrittura, ed in particolare le scritture possono
avvenire solo in append. Se viene passato il flag O_LOCK (eventualmente in OR con O_CREATE) il file viene
aperto e/o creato in modalità locked, che vuol dire che l’unico che può leggere o scrivere il file ‘pathname’ è il
processo che lo ha aperto. Il flag O_LOCK può essere esplicitamente resettato utilizzando la chiamata unlockFile,
descritta di seguito.
Ritorna 0 in caso di successo, -1 in caso di fallimento, errno viene settato opportunamente.
*/
        switch(client_op->flags) {
            case O_CREATE:
                if (icl_hash_find(w_args->hTable, (void*) client_op->pathname) != NULL) { // != NULL -> file esiste
                    int feedback = FAILED;
                    SYSCALL_RETURN("writen", r, writen(connFd, &feedback, sizeof(int)), "Il file esiste già!\n", ""); //tornerà un feedback d'errore
                } 
                else { //Creo il file
                    icl_hash_insert(w_args->hTable, client_op->pathname, (void*)NULL); //per ora NULL, poi vedere se stanziare memoria per buffer
                    printf("File creato correttamente\n");
                    int feedback = SUCCESS;
                    SYSCALL_RETURN("writen", r, writen(connFd, &feedback, sizeof(int)), "Impossibile mandare feedback al client!\n", "");
                }
                break;
            case O_LOCK:
                printf("Flag non supportato\n");
                int feedback = FAILED;
                SYSCALL_RETURN("writen", r, writen(connFd, &feedback, sizeof(int)), "Errore - Impossibile mandare feedback\n", ""); //tornerà un feedback d'errore
                break;
            default:
                printf("Flag non specificato\n");
                int feedback = FAILED;
                SYSCALL_RETURN("writen", r, writen(connFd, &feedback, sizeof(int)),"Errore - Impossibile mandare feedback\n", ""); //tornerà un feedback d'errore
                break;
        }
            break;
        case READFILE:
        /*Legge tutto il contenuto del file dal server (se esiste) ritornando un puntatore ad un'area allocata sullo heap nel
parametro ‘buf’, mentre ‘size’ conterrà la dimensione del buffer dati (ossia la dimensione in bytes del file letto). In
caso di errore, ‘buf‘e ‘size’ non sono validi. Ritorna 0 in caso di successo, -1 in caso di fallimento, errno viene
settato opportunamente.
*/
        void *buf = icl_hash_find(w_args->hTable, client_op->pathname);  
        if (buf = NULL) {
            fprintf(stderr, "Errore, file non trovato\n");
            server_rep->reply_code = FAILED; //-1
            SYSCALL_RETURN("writen", r, writen(connFd, server_rep, sizeof(server_rep)),"Errore - Impossibile rispondere al client\n", ""); //tornerà un feedback d'errore
        }
        server_rep->reply_code = SUCCESS;
        strcpy(server_rep->data, buf);
        SYSCALL_RETURN("writen", r, writen(connFd, server_rep, sizeof(server_rep)),"Errore - Impossibile rispondere al client\n", "");
            break;
        case READNFILES:
            break;
        case WRITEFILE:
            break;
        case APPENDTOFILE:
            break;
        default:
            break;
    }
}




int main (int argc, char **argv) {
    icl_hash_t *hTable = NULL;
    hTable = icl_hash_create(20,NULL, NULL);
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
