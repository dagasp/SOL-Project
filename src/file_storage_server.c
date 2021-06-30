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
static icl_hash_t *hTable;

void initialize_table () {
    hTable = icl_hash_create(20, NULL, NULL);
}

void threadWorker(void *arg) {
    assert(arg);
    w_args = arg;
    long connFd = w_args->sock_fd;
    int r;
    server_reply *server_rep = NULL; 
    client_operations *client_op = NULL;
    if ((r = readn(connFd, client_op, sizeof(server_rep))) < 0) {
        fprintf(stderr, "Impossibile leggere dal client\n");
        return;
    }
    int op_code = client_op->op_code;
    switch (op_code) {
        case OPENFILE:
        switch(client_op->flags) {
            case O_CREATE:
                if (icl_hash_find(hTable, (void*) client_op->pathname) != NULL) { // != NULL -> file esiste
                    int feedback = FAILED;
                    if ((r = writen(connFd, &feedback, sizeof(int))) < 0)
                        break;
                } 
                else { //Creo il file
                    icl_hash_insert(hTable, client_op->pathname, (void*)NULL); //per ora NULL, poi vedere se stanziare memoria per buffer
                    printf("File creato correttamente\n");
                    int feedback = SUCCESS;
                    if ((r = writen(connFd, &feedback, sizeof(int))) < 0)
                        break;
                }
                break;
            case O_LOCK:
                printf("Flag non supportato\n");
                int feedback = FAILED;
                if ((r = writen(connFd, &feedback, sizeof(int))) < 0)
                        break;
                break;
            default:
                printf("Flag non specificato\n");
                int fb = FAILED;
                if ((r = writen(connFd, &fb, sizeof(int))) < 0)
                        break;
                break;
        }
            break;
        case READFILE:    ;
        void *buf = icl_hash_find(hTable, client_op->pathname);
        if (buf == NULL) {
            fprintf(stderr, "Errore, file non trovato\n");
            server_rep->reply_code = FAILED; //-1
            if ((r = writen(connFd, server_rep, sizeof(server_rep))) < 0)
                break;
        }
        server_rep->reply_code = SUCCESS;
        strcpy(server_rep->data, buf);
        if ((r = writen(connFd, server_rep, sizeof(server_rep))))
            break;
        case READNFILES: {
        /*Richiede al server la lettura di ‘N’ files qualsiasi da memorizzare nella directory ‘dirname’ lato client. Se il server
ha meno di ‘N’ file disponibili, li invia tutti. Se N<=0 la richiesta al server è quella di leggere tutti i file
memorizzati al suo interno. Ritorna un valore maggiore o uguale a 0 in caso di successo (cioè ritorna il n. di file
effettivamente letti), -1 in caso di fallimento, errno viene settato opportunamente.
*/      int n_of_files;
        if ((r = readn(connFd, &n_of_files, sizeof(int))) < 0)
            break;
        int available_files = get_n_entries(hTable);
        if (n_of_files <= 0) { //Il server deve leggere tutti i file
        int n_stored_files = available_files;
        if ((r = writen(connFd, &n_stored_files, sizeof(int))) < 0) //Dico al client quanti file invierò
                    break;
            while (n_stored_files > 0) {
                //implementare funzione che legge tutti i files disponibili
                n_stored_files--;
            }
        }
        if (available_files < n_of_files) {
            //deve inviare tutti i files disponibili
        } 
        //deve inviare N files
        }
        case WRITEFILE:
            break;
        case APPENDTOFILE:
            break;
        default:
            break;
    }
}




int main (int argc, char **argv) {
    initialize_table();
    if ((conf = read_config("./test/config.txt")) == NULL) {
        fprintf(stderr, "Errore nella lettura del file config.txt\n");
        exit(EXIT_FAILURE);
    }
    //int num_of_threads_in_pool;
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
