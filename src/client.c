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
    printf("Usage: %s -> -h -f [filename] -w [fileName] -r [file1][file2] -R[n] -d dirname -t [time] -p\n", program_name); //Comandi supportati
}

fqueue *queue; //Coda delle richieste da inviare al server
msg msg_t; //Struct di un messaggio - contiene data e size
msg dir_name; //Struct di un dirname
static int dFlag = 0, pFlag = 0; //Flag per sapere se ho letto -d o -p
unsigned int sleep_time = 0;

/*
*Funzione che tokenizza i parametri per il comando -r, togliendo la virgola e splittando la stringa in N stringhe
*@param args - la stringa da tokenizzare
*@param hM - puntatore che verrà modificato assegnandogli il numero delle stringhe lette
*/
char ** tokenize_args(char *args, unsigned int *hM) {
    size_t size = 1;
    char *p = args;
    while (*p != '\0') {
        if (*p == ',')
            size++;
        ++p; 
    }
    *hM = size;
    size+=1;
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
    return arg;
}


/*
* Invia le richieste al server sfruttando le API del client
*
*/
void send_request () {
    //int err_code;
    node *n;
    memset(&msg_t, 0, sizeof(msg_t));
    n = pop(queue);  
    char opt = n->op_code;
    switch (opt) {
        case 'r': { //Invio richiesta al server di lettura dei files tramite API readFile
            size_t size;
            char *path = n->data;
            if ((openFile(path, 3) != 0)) { //openFile senza specificare flags
                printf("openFile: Impossibile aprire il file\n");
            }

            /*Debug openFile con flag O_CREATE*/
            /*if ((err_code = openFile((char*)n->data, 0) != 0)) {
                printf("Impossibile aprire il file\n");
                break;
            }*/


            else {
                if (pFlag != 0)
                    printf("openFile: File aperto\n");
            }    
            msg_t.data = malloc(sizeof(char)*MAX_FILE_SIZE);
            if (!msg_t.data) {
                fprintf(stderr, "Errore nella malloc\n");
                break;
            }
           
            if  ((readFile(path, (void**) (&msg_t.data),&size) == 0)) {
                if (pFlag != 0)
                    printf("readFile: Letti %ld bytes\n", size);
            }
            else {
                if (pFlag != 0)
                    printf("readFile: Impossibile leggere il file\n");
                break;
            }
            
            /*Debug Append*/

            /*char *append = "Incredibile prova di append, fantastica";
            if (appendToFile("pippo", (void*)append, 40, "files") == 0)
                printf("Agg appis\n");
            else
                printf("Impossibile appendere\n");*/
    
            if (dir_name.data) { //Se non è NULL è perchè -d ci ha scritto dentro qualcosa, devo salvare il file in locale
                if (writeToFile((char*)n->data, msg_t.data, dir_name.data) != 0) {
                    if (pFlag != 0)
                        fprintf(stderr, "writeToFile: Non è stato possibile scrivere sui files\n");
                    free(msg_t.data);
                    break;
                }
                else {
                    if (pFlag != 0)
                        printf("writeToFile: File salvato in locale\n");
                }
            }
            if (closeFile(path) == 0) {
                if (pFlag != 0)
                    printf("closeFile: File chiuso correttamente\n");
            } 
            else {
                if (pFlag != 0)
                    printf("closeFile: Non è stato possibile chiudere il file\n");
            }
            free(msg_t.data);
            break;
        }
        case 'R': {//Invio richiesta al server di lettura di N files
            int n_files_to_read = *((int*)&n->data);
            int how_many;
            how_many = readNFiles(n_files_to_read, dir_name.data);
            if (how_many < 0) {
                if (pFlag != 0)
                    fprintf(stderr, "readNFiles: Non è stato possibile leggere i files dal server\n");
            }
            else {
                if (pFlag != 0)
                    printf("readNFiles: Letti correttamente %d file\n", how_many);
            }
            break;
        }
        case 'w': {
            int rep = writeFile(n->data);
            if (rep == 0) {
                if (pFlag != 0) {
                    printf("writeFile: File caricato correttamente\n");
                }
            }
            else {
                if (pFlag != 0)
                    fprintf(stderr, "writeFile: Non è stato possibile caricare il file nel server\n");
            }
            break;
        }
    }
    free(n);
}

int main(int argc, char **argv) {
    char **to_do = NULL;
    int to_free = 0;
    queue = create();
    long n_files = 0;
     if (argc == 1) {
	    print_usage(argv[0]);
	    exit(EXIT_FAILURE);
        }
    char *sock_name = NULL;
    int opt;
    int rFlag = 0, RFlag = 0;
    while ((opt = getopt(argc, argv, "hf:w:W:D:d:r:R::t:a:l:u:c:p")) != -1) {
        switch(opt) {
            case 'h': {
                print_usage(argv[0]);
                return 0;
            }
            case 'f': {
                sock_name = optarg;
                break;
            }
            case 'r': {
                unsigned int how_many;
                to_do = tokenize_args(optarg, &how_many);
                int i = 0;
                to_free = how_many;
                while (how_many > 0) {
                    insert(queue, 'r', (void*)to_do[i]);
                    i++;
                    how_many--;
                }
                //insert(queue, 'r', (void*)data);
                rFlag = 1;
                i = 0;
                break;
            }
            case 'R': {
                insert(queue, 'R', (void*)n_files);
                RFlag = 1;
                break;
            }
            case 'w':
                insert(queue, 'w', (void*)optarg);
                break;
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
                break;
            case 't':
            if (isNumber(optarg, (long*)&sleep_time) != 0) {
                fprintf(stderr, "Errore - il comando -t richiede un numero\n");
                break;
            }
            if (sleep_time < 0) {
                fprintf(stderr, "Errore - il comando -t richiede un numero positivo\n");
                break;
            }
                break;
            default: 
                printf("Opzione non supportata\n");
                break;
        }
    }
    //--- Connetto al socket --- 
    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);
    time.tv_sec += 5;
    if (openConnection(sock_name, 1000, time) == 0) {
        if (pFlag != 0)
            printf("openConnection: Client connesso\n");
    }
    else {
        if (pFlag != 0)
            printf("openConnection: Impossibile connettersi\n");
        return -1;
    }
    while (queue->head != NULL) { //Fino a quando la coda delle richieste non è vuota
        send_request(); //Invio la richiesta
        usleep(sleep_time*1000); //Mi fermo per il tempo specificato in -t
    }
    if (closeConnection(sock_name) == 0) 
    {
        if (pFlag != 0)
            printf("closeConnection: Connessione chiusa\n");
    }
    else {
        if (pFlag != 0)
            printf("closeConnection: Impossibile chiudere la connessione\n");
    }
    int i = 0;
    
    /*Clean della memoria occupata dall'array di stringhe usato per il parsing degli argomenti*/
    while (to_free > 0) {
        free(to_do[i]);
         i++;
        to_free--;
    }
    free(to_do);
    free(queue);
    return 0;
}