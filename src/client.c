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
config_file *config; //File di config -- contiene il nome del sockname a cui connettersi e la directory in cui salvare i files
msg msg_t;
msg dir_name;

char ** tokenize_args(char *args, size_t *how_many) {
    size_t size;
    char *p = args;
    while (*p != '\0') {
        if (*p == ',')
            size++;
        ++p; 
    }
    *how_many = size;
    char **arg = malloc(sizeof(char*)*size);
    if (!arg) {
        fprintf(stderr, "Errore nella malloc\n");
        return NULL;
    }
    char *token = strtok(args, ",");
    int i = 0;
    while (token) {
        arg[i] = strdup(token);
        token = strtok(NULL, ",");
        i++;
    }
    //*(arg + index) = 0;
    return arg;
}

void send_request (int dFlag, int pFlag) {
    //int err_code;
    node *n;
    memset(&msg_t, 0, sizeof(msg_t));
    n = pop(queue);  
    char opt = n->op_code;
    switch (opt) {
        case 'r': { //Invio richiesta al server di lettura dei files tramite API readFile
            printf("Sono nella readFile lato client\n");
            printf("OP CODE: %c\n", opt);
            int err_code;
            size_t size;
            if ((err_code = openFile((char*)n->data, 3) != 0)) { //Prova openFile senza specificare flags
                printf("Impossibile aprire il file\n");
                break;
            }
            /*if ((err_code = openFile((char*)n->data, 0) != 0)) {
                printf("Impossibile aprire il file\n");
                break;
            }*/
            else    
                printf("File aperto\n");
            msg_t.data = malloc(sizeof(char)*BUFSIZE);
            if (!msg_t.data) {
                fprintf(stderr, "Errore nella malloc\n");
                break;
            }
           
            if  ((err_code = readFile((char*)n->data, (void**) (&msg_t.data),&size) == 0)) {
                 printf("Letti %ld bytes\n", size);
            }
            printf("FILE RICEVUTO: %s\n", msg_t.data);
            
            /*Debug Append*/

           /* char *append = "Incredibile prova di append, fantastica";
            if (appendToFile("pippo", (void*)append, 40, "boh") == 0)
                printf("Agg appis\n");
            else
                printf("Impossibile appendere\n");*/
            
            
            if (dFlag != 0) {
                if (writeToFile((char*)n->data, msg_t.data, dir_name.data) != 0) {
                    fprintf(stderr, "Non è stato possibile scrivere sui files\n");
                    free(msg_t.data);
                    break;
                }
            }
            if (closeFile(n->data) == 0) printf("File chiuso correttamente\n");
            else printf("Non è stato possibile chiudere il file\n");
            free(msg_t.data);
            break;
        }
        case 'R': {//Invio richiesta al server di lettura di N files
            //int err;
            printf("Sono nella readNFiles\n");
            printf("OP CODE: %c\n", opt);
            int n_files_to_read = *((int*)&n->data);
            if (readNFiles(n_files_to_read, dir_name.data) < 0) {
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
    //char *sock_name;
    int opt;
    int rFlag = 0, RFlag = 0, dFlag = 0, pFlag = 0;
    //Da implementare: -h, -f, -r, -R, -t, -p
    while ((opt = getopt(argc, argv, "hf:w:W:D:d:r:R::t::l:u:c:p")) != -1) {
        switch(opt) {
            case 'h': {
                print_usage(argv[0]);
                return 0;
            }
            case 'f': {
                //sock_name = optarg;
                break;
            }
            case 'r': {
                printf("E' stata chiesta la readFile\n");
                data = optarg;
                insert(queue, 'r', (void*)data);
                rFlag = 1;
                //inserire richiesta in lista -
                break;
            }
            case 'R': {
                //int err;
                if (isNumber(optarg, &n_files) != 0) {
                    fprintf(stderr, "Non è stato inserito un numero\n");
                    //break;
                }
                printf("%ld\n", n_files);
                insert(queue, 'R', (void*)n_files);
                RFlag = 1;
                break;
            }
            case 'd':
                if (rFlag == 0 && RFlag == 0) {
                    fprintf(stderr, "Errore, il comando -d va usato congiuntamente a -r o -R\n");
                    return -1;
                }
                dFlag = 1;
                dir_name.data = optarg;
                break;
            case 'p':
                pFlag = 1;
            default: 
                printf("Opzione non supportata\n");
                break;
        }
    }
    if ((config = read_config("../test/config.txt")) == NULL) {
        fprintf(stderr, "Errore nella lettura del file config.txt\n");
        exit(EXIT_FAILURE);
    } //Lettura parametri dal file config
    //--- Connetto al socket --- 
    //printf("%s\n", config->sock_name);
    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);
    time.tv_sec += 5;
    openConnection(config->sock_name, 1000, time);
    while (queue->head != NULL) { //Fino a quando la coda delle richieste non è vuota
        send_request(dFlag, pFlag);
    }
    if (closeConnection(config->sock_name) == 0)
        printf("Connessione chiusa\n");
    else 
        printf("Impossibile chiudere la connessione\n");
    free(config);
    free(queue);
    return 0;
}