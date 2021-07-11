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
#include "list.h"

/**
 *  @struct sigHandlerArgs_t
 *  @brief struttura contenente le informazioni da passare al signal handler thread
 *
 */
typedef struct {
    sigset_t     *set;           /// set dei segnali da gestire (mascherati)
    int           signal_pipe;   /// descrittore di scrittura di una pipe senza nome
} sigHandler_t;

//Configurazione server da file
static config_file *conf;

//Hash table dei file e la sua lock
static icl_hash_t *hTable = NULL; 
pthread_mutex_t filesTable = PTHREAD_MUTEX_INITIALIZER;

list *open_file; //Lista dei file aperti da tutti i client
pthread_mutex_t openFileLock = PTHREAD_MUTEX_INITIALIZER;

list *file_list; //Lista di tutti i file
pthread_mutex_t fileListLock = PTHREAD_MUTEX_INITIALIZER;

//Pool di threadpool
threadpool_t *pool = NULL;

//Set 
fd_set set, tmpset;


//Funzione per aggiornare il massimo dei descrittori nel set
int updatemax(fd_set set, int fdmax) {
    for(int i=(fdmax-1);i>=0;--i)
		if (FD_ISSET(i, &set)) return i;
    	assert(1==0);
    	return -1;
}


/*
*Controlla che il file storage possa contenere almeno un altro file, in caso contrario toglie il primo file inserito (FIFO)
*
*/
int check_numOfFiles_FIFO() { 
    if (hTable->nentries + 1 > hTable->max_files) { //Devo eliminare un file
        printf("Ho raggiunto il massimo numero di file, elimino il primo inserito\n");
        LOCK(&fileListLock);
        char *to_delete = get_last_file(file_list); //Prendo il primo inserito - FIFO
        if (!to_delete) return -1;
        if (icl_hash_delete(hTable, (void*)to_delete, NULL, free) == 0) { //Lo tolgo sia dalla hashTable che dalla lista dei file 
            printf("File eliminato correttamente dalla tabella\n");
            delete_last_element(&file_list);
            UNLOCK(&fileListLock);
            return 0;
        }
    }
    return -1; //Non ho avuto bisogno di eliminare file
}

/*
*Controlla che il file storage abbia spazio in memoria per un nuovo file, in caso contrario toglie il primo file inserito, ricorsivamente,
*fino a quando non c'è abbastanza spazio (FIFO)
*Valore di ritorno: il numero di file eliminati
*@param new_size - il peso in byte del file da inserire
*/

static int how_many = 0;
int check_memory_FIFO(size_t new_size) {
    if (new_size == 0) return 0; //Se il file da inserire è vuoto, esco per inserirlo subito
    if (new_size > hTable->max_size) {
        fprintf(stderr, "Errore, il nuovo contenuto sarebbe più grande della dimensione massima della memoria\n");
        return -1;
    } 
    if (get_current_size(hTable) + new_size > hTable->max_size) { //Memoria superata-> ho bisogno di liberare spazio
        printf("CURR SIZE: %ld\n", hTable->curr_size);
        printf("Ho raggiunto la dimensione massima della memoria, elimino qualcosa...\n");
        LOCK(&fileListLock);
        char *to_delete = get_last_file(file_list);
        //printf("TO DELETE: %s\n", to_delete);
        if (!to_delete) {
            UNLOCK(&fileListLock);
            return -1;
        }
        int rep;
        rep = icl_hash_delete(hTable, (void*)to_delete, NULL, free); //Lo tolgo sia dalla hashTable che dalla lista dei file 
        if (rep != 0) {
            fprintf(stderr, "Errore - non è stato possibile eliminare il file\n");
            UNLOCK(&fileListLock);
            return -1;
        }
        printf("File eliminato correttamente\n");
        how_many++;
       // printf("BEFORE-----------------------------\n");
        //print_q(file_list);
        delete_last_element(&file_list);
       // printf("AFTER-----------------------------\n");
        //print_q(file_list); 
        UNLOCK(&fileListLock); 
        check_memory_FIFO(new_size);  
    }
    return how_many;
}

//WorkerFunction eseguita dal main thread
void threadWorker(void *arg) {
    assert(arg);
    long* args = (long*)arg;
    long connFd = args[0]; //Descrittore del socket
	int req_pipe = (int) args[1]; //Pipe di comunicazione col main thread
    free(arg);
    int r;
    server_reply server_rep;        //Struct in risposta al client
    client_operations client_op;    //Struct che legge i messaggi inviati dal client
    msg msg;                        //Struct per la gestione dei messaggi
    memset(&client_op, 0, sizeof(client_operations));
    memset(&server_rep, 0, sizeof(server_reply));
    memset(&msg, 0, sizeof(msg));

    if ((r = readn(connFd, (void*)&client_op, sizeof(client_operations))) < 0) { //Leggo la richiesta
        fprintf(stderr, "Impossibile leggere dal client\n");
        return;
    }
    if (r == 0) { //Il client ha finito le richieste -- chiudo la connessione
        close(connFd);
        return;
    }
    //printf("OP CODE RICEVUTO DAL SERVER: %d\n", client_op.op_code);
    //printf("PATHNAME RICEVUTO: %s\n", client_op.pathname);
    int op_code = client_op.op_code;
    switch (op_code) {
     case OPENFILE: //AGGIUNGERE O_CREATE IN OR CON O_LOCK
        //printf("FLAGS: %d\n", client_op.flags);
           if (client_op.flags == O_CREATE) {
                if (icl_hash_find(hTable, (void*) client_op.pathname) != NULL) { // != NULL -> file esiste
                    //printf("File esiste + OCREATE\n");
                    int feedback = FAILED;
                    if ((r = writen(connFd, &feedback, sizeof(int))) < 0) {
                        perror("writen");
                        break;
                    }
                    break;
                } 
                else { //Creo il file
                    if (icl_hash_insert(hTable, client_op.pathname, client_op.data) != NULL) {
                        printf("File creato correttamente\n");
                        LOCK(&fileListLock);
                        if (insert_file(&file_list, client_op.pathname) != 0) //Metto il file nella lista di tutti i file 
                            printf("File già presente in coda, non inserito\n");
                        else printf("File inserito anche in coda\n");
                        UNLOCK(&fileListLock);
                        LOCK(&openFileLock);
                        put_by_key(&open_file, client_op.pathname, client_op.client_desc); //Metto il file nella lista dei file aperti da quel descrittore
                        UNLOCK(&openFileLock);
                        int feedback = SUCCESS;
                        if ((r = writen(connFd, &feedback, sizeof(int))) < 0) {
                            perror("writen");
                            break;
                        }
                    break;
                    } 
                }
           }    
            else if (client_op.flags == O_LOCK) {
                printf("Flag non supportato\n");
                int feedback = FAILED;
                if ((r = writen(connFd, &feedback, sizeof(int))) < 0) {
                    perror("writen");
                    break;
                }  
                break;
            }
            else if (client_op.flags == O_CREATE_OR_O_LOCK) {
                printf("Flag non supportato\n");
                int feedback = FAILED;
                if ((r = writen(connFd, &feedback, sizeof(int))) < 0) {
                    perror("writen");
                    break;
                }  
                break;
            }
            else { //Nessun flag specificato - Cerco il file, se esiste lo apro
                if (icl_hash_find(hTable, (void*) client_op.pathname) == NULL) { // == NULL -> file non esiste
                    int feedback = FAILED;
                    if ((r = writen(connFd, &feedback, sizeof(int))) < 0) {
                        perror("writen");
                        break;
                    }
                } 
                else { //File esiste
                    printf("Trying to open the file...\n");
                    LOCK(&openFileLock);
                    put_by_key(&open_file,client_op.pathname,client_op.client_desc); //Lo inserisco nella lista dei files aperti
                    UNLOCK(&openFileLock);
                    //print_q(open_file);
                        printf("File aperto\n");
                        int fb = SUCCESS;
                        printf("FBFBFB %d\n", fb);
                        if ((r = writen(connFd, &fb, sizeof(int))) < 0) {
                            perror("writen");
                            break;
                        }      
                }
                break;
            }
        case READFILE:
        msg.data = icl_hash_find(hTable, client_op.pathname); //Prendo il file dalla hashTable
        if (msg.data == NULL) {
            fprintf(stderr, "Errore, file non trovato\n");
            server_rep.reply_code = FAILED; //-1
            if ((r = writen(connFd, (void*)&server_rep, sizeof(server_reply))) < 0) {
                perror("writen");
                break;
            }
            break;
        }
        LOCK(&openFileLock);
        if (list_contain_file(open_file, client_op.pathname, client_op.client_desc) == 0) { //Se il file è nella lista dei file aperti da quel client, lo copio
            //print_q(open_file);
            UNLOCK(&openFileLock);
            printf("File is open - SUCCESS\n");
            msg.size = strnlen(msg.data, MAX_FILE_SIZE);
            printf("MSG SIZE IS: %ld\n", msg.size);
            server_rep.reply_code = SUCCESS;
            memcpy(server_rep.data, msg.data, msg.size);
            if ((r = writen(connFd, (void*)&server_rep, sizeof(server_reply))) < 0) {
                perror("writen");
                break;
            }
        }
        else { //Il file non era in lista, non è stato aperto, non è stato possibile leggerlo
            UNLOCK(&openFileLock);
            printf("File is closed - FAILED\n");
            server_rep.reply_code = FAILED;
            if ((r = writen(connFd, (void*)&server_rep, sizeof(server_reply))) < 0) {
                perror("writen");
                break;
            }
        }
        break;
        case READNFILES: { 
        int n_of_files;
        printf("IL CLIENT VUOLE LEGGERE %d FILE\n", client_op.files_to_read);
        n_of_files = client_op.files_to_read;
        int available_files = get_n_entries(hTable);
        if (n_of_files <= 0 || available_files < n_of_files) { //Il server deve leggere tutti i file disponibili
            int n_stored_files = available_files; 
            if ((r = writen(connFd, &n_stored_files, sizeof(int))) < 0) { //Dico al client quanti file invierò
                    perror("writenReadN1");
                    break;     
            }
            if (icl_hash_dump(connFd, hTable, available_files) == 0) { //Operazione andata a buon fine, invio feedback positivo
                printf("OK\n");
            }
            else printf("NOT OK \n");
        }
        else { //deve inviare gli N files richiesti
        if ((r = writen(connFd, &n_of_files, sizeof(int))) < 0) { //Dico al client quanti file invierò
                    perror("writenReadN2");
                    break;     
            }
            if (icl_hash_dump(connFd, hTable, n_of_files) == 0) { //File letti correttamente
                printf("ok\n");  
            }
            else printf("NOT OK \n");
        }        
        }
        case WRITEFILE:
            break;
        case APPENDTOFILE:
       // icl_hash_dump_2(stdout, hTable);
            LOCK(&openFileLock);
            if (list_contain_file(open_file, client_op.pathname, client_op.client_desc) != 0) { //File non aperto dal client
                fprintf(stderr, "File non aperto, impossibile effettuare l'append\n");
                UNLOCK(&openFileLock);
                break;
            }
            UNLOCK(&openFileLock);
            check_memory_FIFO(client_op.size);
            //icl_hash_dump_2(stdout, hTable);
                if (append(hTable, (void*)client_op.pathname, client_op.data, client_op.size) == 0) {
                    printf("HO APPESO\n");
                    icl_hash_dump_2(stdout, hTable);
                    int fb = SUCCESS;
                    if ((r = writen(connFd, &fb, sizeof(int))) < 0) {
                            perror("writen positive feedback append");
                            break;
                        }  
            }
                else {
                    int fb = FAILED;
                    printf("NON HO APPESO\n");
                    //icl_hash_dump_2(stdout, hTable);
                    if ((r = writen(connFd, &fb, sizeof(int))) < 0) {
                            perror("writen negative feedback append");
                            break;
                    }  
                } 
            break;
        case CLOSEFILE:
                LOCK(&openFileLock);
                if (delete_by_key(&open_file, client_op.pathname, client_op.client_desc) == 0) { //File chiuso correttamente 
                    UNLOCK(&openFileLock);
                    int fb = SUCCESS;
                    if ((r = writen(connFd, &fb, sizeof(int))) < 0) {
                            perror("writen positive feedback closeFile");
                            break;
                    }
                } 
                else {
                    UNLOCK(&openFileLock);
                    int fb = FAILED;
                    if ((r = writen(connFd, &fb, sizeof(int))) < 0) {
                            perror("writen negative feedback closeFile");
                            break;
                    } 
                }  
                
            
        /*case CLOSECONNECTION: ;
           // FD_CLR(connFd, &set);
            int fb = SUCCESS;
            if ((r = writen(connFd, &fb, sizeof(int))) < 0) {
                perror("write positive feedback closeConnection");
                break;
            }
            break;*/
        default:
            break;
    }
    //Scrivo sulla pipe di comunicazione col main thread il descrittore per segnalare che il worker ha finito
    if (writen(req_pipe, &connFd, sizeof(long)) == -1) {
		perror("write pipe");
		return;
	}
}

//Funzione che prepara il main thread alla chiusura distruggendo le strutture e liberando la memoria associata
void destroy_everything(int force) {
    icl_hash_destroy(hTable, NULL, free);
    destroyThreadPool(pool, force);  // notifico che i thread dovranno uscire
    unlink(conf->sock_name);
    list_destroy(&file_list);
    list_destroy(&open_file);
    free(conf);
}

// SigHandler Thread
static void *sigHandler(void *arg) {
    sigset_t *set = ((sigHandler_t*)arg)->set;
    int fd_pipe   = ((sigHandler_t*)arg)->signal_pipe;

    for( ;; ) {
	    int sig;
	    int r = sigwait(set, &sig);
	    if (r != 0) {
	        errno = r;
	        perror("FATAL ERROR 'sigwait'");
	        return NULL;
	    }

	    switch(sig) {
	        case SIGINT:
	        case SIGQUIT:
                printf("Ricevuto segnale %s, avvio chiusura immediata\n", (sig==SIGINT) ? "SIGINT" : "SIGQUIT");
                destroy_everything(1); //Con '1' termina subito e non accetta nuove richieste
                close(fd_pipe);
                pthread_exit(NULL);
	        case SIGHUP:
	            printf("Ricevuto segnale %s, aspetto i thread attivi e termino\n", "SIGHUP");
                destroy_everything(0); //Con '0' aspetta i thread pendenti e poi termina (Non accetta nuove connessioni)
	            close(fd_pipe);  // notifico il listener thread della ricezione del segnale
                //pthread_exit(NULL);
                return NULL;
	default:  ; 
	    }
    }
    pthread_exit(NULL);	   
}

int main (int argc, char **argv) {
    //Lettura dei parametri dal file di configurazione
    if ((conf = read_config("./test/config.txt")) == NULL) {
        fprintf(stderr, "Errore nella lettura del file config.txt\n");
        exit(EXIT_FAILURE);
    }
    //conf->sock_name[strcspn(conf->sock_name, "\r\n")] = '\0';
    //printf("%s", conf->sock_name);
    //hTable = icl_hash_create(conf->num_of_files, NULL, NULL, conf->memory_space, conf->num_of_files);
    open_file = NULL;
    file_list = NULL;
    //icl_entry_t *entry


    /*Debug inserimento file server*/
    //printf("SIZE: %ld\n", sizeof(hTable)/MB);
    insert_file(&file_list, "/mnt/c/Users/davyx/files/gianni");
    insert_file(&file_list, "/mnt/c/Users/davyx/files/minnie");
    insert_file(&file_list, "pippos");
    insert_file(&file_list, "/mnt/c/Users/davyx/files/giannis");
    insert_file(&file_list, "pippo");
    insert_file(&file_list, "albertino");
    //insert_tail(&file_list, "Ultimo");
    //print_q(file_list);
    //delete_last_element(&file_list);
    //print_q(file_list);
    /*print_q(file_list);
    printf("FILE IN CODA: %s\n", get_last_file(file_list));
    delete_by_key(&file_list, "/mnt/c/Users/davyx/files/minnies");
    print_q(file_list);*/
    hTable = icl_hash_create(conf->num_of_files, NULL, NULL, conf->memory_space, conf->num_of_files);
    char *file_gianni = strdup("no vabbè");
    char *file_minnie = strdup("sempre meglio");
    char *file_pippos = strdup("ancora più bello");
    char *file_giannis = strdup("fantastico");
    char *file_pippo = strdup("wow che contenuto");
    char *file_albertino = strdup("Ciao sono albertino, questo è il mio file, bellissimo.");

    if (check_numOfFiles_FIFO() != 0) {
        icl_hash_insert(hTable, "pippo", (void*)file_pippo);
    }
    if (check_numOfFiles_FIFO() != 0) {
        icl_hash_insert(hTable, "/mnt/c/Users/davyx/files/gianni", (void*)file_gianni);
    }
    if (check_numOfFiles_FIFO() != 0) {
        icl_hash_insert(hTable, "/mnt/c/Users/davyx/files/minnie", (void*)file_minnie);
    }
    if (check_numOfFiles_FIFO() != 0) {
        icl_hash_insert(hTable, "pippos", (void*)file_pippos);
    }
    if (check_numOfFiles_FIFO() != 0) {
        icl_hash_insert(hTable, "/mnt/c/Users/davyx/files/giannis", (void*)file_giannis);
    }
    if (check_numOfFiles_FIFO() != 0) {
        icl_hash_insert(hTable, "albertino", (void*)file_albertino);
    }
    //printf("%s\n",get_last_file(file_list));
    //check_memory_FIFO(34);
    //icl_hash_insert(hTable, "/mnt/c/Users/davyx/files/minnies", "questo è un contenuto fantastico");

    //put_by_key(&open_file, "pippo", 5);

    //icl_hash_dump_2(stdout, hTable);
    /*insert_by_key(open_file, "albertino", 5);
    insert_by_key(open_file, "pippo", 5);
    insert_by_key(open_file, "gianni", 5);
    print_q(open_file);
    remove_by_key(open_file, "uberti");
    remove_by_key(open_file, "ubertini");
    printf("--------------------------\n");
    print_q(open_file);
    */
    /*Fine debug*/
    //icl_hash_dump(stdout, hTable);


    int num_of_threads_in_pool = conf->num_of_threads;
    
    //Maschere per i segnali 
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT); 
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGHUP);
    
    //Applico la maschera
    if (pthread_sigmask(SIG_BLOCK, &mask,NULL) != 0) {
	    fprintf(stderr, "FATAL ERROR\n");
        exit(EXIT_FAILURE);
    }
    // ignoro SIGPIPE per evitare di essere terminato da una scrittura su un socket
    struct sigaction s;
    memset(&s,0,sizeof(s));    
    s.sa_handler=SIG_IGN;
    if ((sigaction(SIGPIPE,&s,NULL)) == -1) {   
	    perror("sigaction");
	    destroy_everything(1);
        return -1;
    } 

    /*
     * La pipe viene utilizzata come canale di comunicazione tra il signal handler thread ed il 
     * thread listener per notificare la terminazione. 
     *
     */
    int signal_pipe[2];
    if (pipe(signal_pipe)==-1) {
		perror("signal pipe");
		destroy_everything(1);
        return -1;
    }

    //Pipe di notifica di una richiesta che è stata servita e il thread è di nuovo pronto per altre richieste
	int pipe_request[2];
	if (pipe(pipe_request) == -1) {
		destroy_everything(1);
        return -1;
	}
    
    /*Thread di gestione dei segnali*/
    pthread_t sighandler_thread;
    sigHandler_t handlerArgs = { &mask, signal_pipe[1] };
   
    if (pthread_create(&sighandler_thread, NULL, sigHandler, &handlerArgs) != 0) {
		fprintf(stderr, "Errore nella creazione del signal handler thread\n");
		destroy_everything(1);
        return -1;
    }
    
    int listenfd;
    if ((listenfd= socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket");
        destroy_everything(1);
		pthread_join(sighandler_thread, NULL);
        return -1;
    }

    /*Inizializzazione socket*/

    struct sockaddr_un serv_addr;
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;    
    strncpy(serv_addr.sun_path, conf->sock_name, strlen(conf->sock_name)+1);

    if (bind(listenfd, (struct sockaddr*)&serv_addr,sizeof(serv_addr)) == -1) {
	    destroy_everything(1);
		pthread_join(sighandler_thread, NULL);
        return -1;
    }
    if (listen(listenfd, MAXBACKLOG) == -1) {
	    destroy_everything(1);
		pthread_join(sighandler_thread, NULL);
        return -1;
    }

    //Creazione di un nuovo threadPool
    pool   = createThreadPool(num_of_threads_in_pool, num_of_threads_in_pool);
    if (!pool) {
		fprintf(stderr, "Errore nella creazione del threadPool\n");
		destroy_everything(1);
		pthread_join(sighandler_thread, NULL);
        return -1;
    }
    
    FD_ZERO(&set);
    FD_ZERO(&tmpset);

    FD_SET(listenfd, &set);        //descrittore del socket
    FD_SET(signal_pipe[0], &set);  // descrittore di lettura della signal_pipe
    FD_SET(pipe_request[0], &set); // descrittore di lettura della pipe_request

    // tengo traccia del file descriptor con id piu' grande
    int fdmax = (listenfd > signal_pipe[0]) ? listenfd : signal_pipe[0];

    volatile long termina = 0;
    while(!termina) {
        tmpset = set;
	// copio il set nella variabile temporanea per la select
	if (select(fdmax+1, &tmpset, NULL, NULL, NULL) == -1) {
	    perror("select");
	    destroy_everything(1);
		pthread_join(sighandler_thread, NULL);
        return -1;
	}
	// Ciclo sugli fd delle richieste per vedere quale è pronto
	for(int i=0; i <= fdmax; i++) {
	    if (FD_ISSET(i, &tmpset)) {
		long connfd;
		if (i == listenfd) { // Nuova richiesta di connessione
		    if ((connfd = accept(listenfd, (struct sockaddr*)NULL ,NULL)) == -1) {
			    destroy_everything(1);
		        pthread_join(sighandler_thread, NULL);
                return -1;
		    }
			else
				printf("Client connesso\n");
			FD_SET(connfd, &set); //Aggiunto fd al set
			if (connfd > fdmax)
				fdmax = connfd;
		}
		else if (i == pipe_request[0]) { //La pipe di comunicazione col worker mi segnala che il thread worker ha servito 
			int n;
			long desc;
			if ((n=readn(pipe_request[0], &desc, sizeof(long))) == -1) {
				perror("read");
				continue;
			}
			//printf("ThreadWorker ha servito\n");
			FD_SET(desc, &set); //Rimetto fd nel set
			if (desc > fdmax)
				fdmax = desc;
		}

		else if (i == signal_pipe[0]) {
		    //La pipe mi segnala che ho ricevuto un segnale, esco ed inizio il protocollo di terminazione
		    termina = 1;
		    break;
		}
		else { //E' una richiesta da eseguire - la mando al thread worker
			long* args = malloc(3*sizeof(long));
		    if (!args) {
		        perror("Errore nella malloc\n");
		        destroy_everything(1);
		        pthread_join(sighandler_thread, NULL);
                return -1;
		    }
			args[0] = connfd;
			args[1] = (int) pipe_request[1]; //Pipe di comunicazione tra main thread e worker
			FD_CLR(i, &set); //Tolgo il suo fd dal set (momentaneamente)
			fdmax = updatemax(set, fdmax); //Aggiorno il massimo tra i descrittori
		    int r = addToThreadPool(pool, threadWorker, (void*)args); //Invio task al threadPool
		    if (r==0) continue; // aggiunto con successo
		    if (r < 0) { //Fallimento
			    fprintf(stderr, "Impossibile aggiungere al threadPool\n");
		    } 
            else { //Non ci sono thread disponibili
			    fprintf(stderr, "Errore - nessun thread disponibile\n");
		    }
		    free(args);
		   	close(connfd);
		    continue;
		}
	    }
	}
    }
    //destroy_everything(0); //libererà la memoria aspettando che tutti i thread finiscano
    pthread_join(sighandler_thread, NULL);
    return 0;    
}
