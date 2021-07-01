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
    printf("Usage: %s -> -h -f [filename] -w [dirname] -W [file1][file2] -D [dirname] -r [file1][file2] -R[n] -d dirname -t [time] -l [file1][file2] -u [file1][file2] -c [file1][file2] -p\n", program_name); //inserire comandi supportati
}

fqueue *queue; //Coda delle richieste da inviare al server
config_file *config;

void configuration () {
    if ((config = read_config("./test/config.txt")) == NULL) {
        fprintf(stderr, "Errore nella lettura del file config.txt\n");
        exit(EXIT_FAILURE);
    }
}

void send_request () {
    //int err_code;
    node *n = malloc(sizeof(node));
    n = pop(queue);
    char opt = n->op_code;
    switch (opt) {
        case 'r': { //Invio richiesta al server di lettura dei files tramite API readFile
            int err_code;
            size_t size;
            char *buf = malloc(sizeof(char)*BUFSIZE);
            if (!buf) {
                fprintf(stderr, "Errore fatale nella malloc\n");
                exit(EXIT_FAILURE); 
             }
            SYSCALL_PRINT("readFile", err_code, readFile(n->data, (void**) (&buf),&size) ,"Errore nella readFile\n","");
            printf("Letti %ld bytes\n", size);
            //da implementare stampa;
            free(buf);
            break;
        }
        case 'R': {//Invio richiesta al server di lettura di N files
            //int err;
            int n_files_to_read = *((int*)&n->data);
            if (readNFiles(n_files_to_read, config->directory) < 0) {
                fprintf(stderr, "Non è stato possibile leggere i files dal server\n");
            }
            break;
        }
    }
    free(n);
}

int main(int argc, char **argv) {
    queue = create();
    char *data = NULL;
    long n_files = 0;
     if (argc == 1) {
	    print_usage(argv[0]);
	    exit(EXIT_FAILURE);
        }
    int opt;
    //Da implementare: -h, -f, -r, -R, -t, -p
    while ((opt = getopt(argc, argv, "hf:w:W:D:r:R:d:t:l:u:c:p")) != -1) { 
        switch(opt) {
            case 'h': {
                print_usage(argv[0]);
                return 0;
            }
            case 'f': {
                //da implementare
                break;
            }
            case 'r': {
                data = optarg;
                insert(queue, 'r', (void*)data);
                //inserire richiesta in lista -
                break;
            }
            case 'R': {
                //int err;
                if (isNumber(optarg, &n_files) != 0) {
                    fprintf(stderr, "Non è stato inserito un numero\n");
                    break;
                }
                insert(queue, 'R', (void*)n_files);
                break;
            }
            default: 
                printf("Opzione non supportata\n");
                break;
        }
    }
    if ((config = read_config("../test/config.txt")) == NULL) {
        fprintf(stderr, "Errore nella lettura del file config.txt\n");
        exit(EXIT_FAILURE);
    }
    //--- Connetto al socket --- 
    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);
    time.tv_sec += 5;
    openConnection(config->sock_name, 1000, time);
    while (queue->head != NULL) { //Fino a quando la coda delle richieste non è vuota
        send_request();
        queue->head = queue->head->next;
    }
    return 0;
    free(queue);
}