#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h> /* ind AF_UNIX */
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>

#include "util.h"
#include "api.h"
#include "queue.h"

void print_usage(const char *program_name) {
    printf("Usage: %s -> ", program_name); //inserire comandi supportati
}

fqueue *queue = create(); //Coda delle richieste da inviare al server
config_file *config;

void configuration () {
    if ((config = read_config("./test/config.txt")) == NULL) {
        fprintf(stderr, "Errore nella lettura del file config.txt\n");
        exit(EXIT_FAILURE);
    }
}

void send_request () {
    int err_code;
    node *n;
    n = pop(queue);
    int opt = n->op_code;
    switch (opt) {
        case 'r': //Invio richiesta al server di lettura dei files tramite API readFile
            int err_code;
            size_t size;
            char *buf = malloc(sizeof(char)*BUFSIZE);
            if (!buf) {
                fprintf(stderr, "Errore fatale nella malloc\n");
                exit(EXIT_FAILURE);
            }
            SYSCALL_RETURN("readFile", err_code, readFile(n->data,&buf,&size) ,"Errore nella readFile\n","");
            printf("Letti %d bytes\n", size);
            //da implementare stampa;
            free(buf);
            break;
        case 'R': //Invio richiesta al server di lettura di N files
            int err;
            int n_files_to_read = (int*) n->data;
            if (readNFiles(n_files_to_read, config->directory));
            break;
    }
}

int main(int argc, char **argv) {
    char *data = NULL;
    long n_files = 0;
     if (argc == 1) {
	    printf("Nessun argomento passato al programma\n");
	    exit(EXIT_FAILURE);
        }
    int opt;
    //Da implementare: -h, -f, -r, -R, -t, -p
    while ((opt = getopt(argc, argv, "h:f:w:W:D:r:R:d:t:l:u:c:p")) != -1) { 
        switch(opt) {
            case 'h':
                print_usage(argv[0]);
                return 0;
            case 'f':
                //da implementare
                break;
            case 'r':
                data = optarg;
                insert(queue, 'r', (void*)data);
                //inserire richiesta in lista -
                break;
            case 'R':
                int err;
                if (isNumber(optarg, &n_files) != 0) {
                    fprintf(stderr, "Non è stato inserito un numero\n");
                    break;
                }
                insert(queue, 'R', (void*)n_files);
                break;
            default: 
                printf("Opzione non supportata\n");
                break;
        }
    }
    //--- Connettere al socket --- 

    while (queue->head != NULL) { //Fino a quando la coda delle richieste non è vuota
        send_request();
        queue->head = queue->head->next;
    }
    return 0;
}