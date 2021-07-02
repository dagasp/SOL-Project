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


/**
 *  @struct sigHandlerArgs_t
 *  @brief struttura contenente le informazioni da passare al signal handler thread
 *
 */
typedef struct {
    sigset_t     *set;           /// set dei segnali da gestire (mascherati)
    int           signal_pipe;   /// descrittore di scrittura di una pipe senza nome
} sigHandler_t;

static config_file *conf;
//static wargs *w_args;
static icl_hash_t *hTable;

void initialize_table () {
    hTable = icl_hash_create(20, NULL, NULL);
}


// funzione eseguita dal signal handler thread
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
	case SIGTERM:
	case SIGQUIT:
	    //printf("ricevuto segnale %s, esco\n", (sig==SIGINT) ? "SIGINT": ((sig==SIGTERM)?"SIGTERM":"SIGQUIT") );
	    close(fd_pipe);  // notifico il listener thread della ricezione del segnale
	    return NULL;
	default:  ; 
	}
    }
    return NULL;	   
}

int updatemax(fd_set set, int fdmax) {
    for(int i=(fdmax-1);i>=0;--i)
		if (FD_ISSET(i, &set)) return i;
    	assert(1==0);
    	return -1;
}

void threadWorker(void *arg) {
    assert(arg);
    long* args = (long*)arg;
    long connFd = args[0];
    //long* termina = (long*)(args[1]);
	int req_pipe = (int) args[1];
    free(arg);
    int r;
    server_reply server_rep;
    client_operations client_op;
    msg msg;
    memset(&client_op, 0, sizeof(client_op));
    memset(&server_rep, 0, sizeof(server_rep));
    memset(&msg, 0, sizeof(msg));
    if ((r = readn(connFd, (void*)&client_op, sizeof(server_rep))) < 0) {
        fprintf(stderr, "Impossibile leggere dal client\n");
        return;
    }
    if (r == 0) { //Il client ha finito le richieste
        close(connFd);
        return;
    }
    printf("OP CODE RICEVUTO DAL SERVER: %d\n", client_op.op_code);
    printf("PATHNAME RICEVUTO: %s\n", client_op.pathname);
    int op_code = client_op.op_code;
    switch (op_code) {
     case OPENFILE:
        //printf("FLAGS: %d\n", client_op.flags);
           if (client_op.flags == O_CREATE) {
                if (icl_hash_find(hTable, (void*) client_op.pathname) != NULL) { // != NULL -> file esiste
                    printf("File esiste + OCREATE\n");
                    int feedback = FAILED;
                    if ((r = writen(connFd, &feedback, sizeof(int))) < 0) {
                        perror("writen");
                        break;
                    }
                    break;
                } 
                else { //Creo il file
                    icl_hash_insert(hTable, client_op.pathname, (void*)NULL); //per ora NULL, poi vedere se stanziare memoria per buffer
                    printf("File creato correttamente\n");
                    int feedback = SUCCESS;
                    if ((r = writen(connFd, &feedback, sizeof(int))) < 0) {
                        perror("writen");
                        break;
                    }
                    break;
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
                    int rep = open_file(hTable, (void*)client_op.pathname); //Apro il file
                    if (rep == 0) { //Apertura file con successo, segnalo
                        printf("File aperto nella HTABLE\n");
                        int fb = SUCCESS;
                        if ((r = writen(connFd, &fb, sizeof(int))) < 0) {
                            perror("writen");
                            break;
                        }      
                    } 
                }
                break;
            }
        case READFILE:
        msg.data = malloc(sizeof(char)*BUFSIZE);
        if (!msg.data) {
            fprintf(stderr, "Errore nella malloc\n");
            break;
        }
        msg.data = icl_hash_find(hTable, client_op.pathname);
        if (msg.data == NULL) {
            fprintf(stderr, "Errore, file non trovato\n");
            server_rep.reply_code = FAILED; //-1
            if ((r = writen(connFd, (void*)&server_rep, sizeof(server_rep))) < 0) {
                free(msg.data);
                perror("writen");
                break;
            }
            free(msg.data);
            break;
        }
        if (is_file_open(hTable, client_op.pathname) == OPEN) { //Se il file è aperto lo copio
            printf("File is open - SUCCESS\n");
            msg.size = strnlen(msg.data, BUFSIZE);
            printf("MSG SIZE IS: %ld\n", msg.size);
            server_rep.reply_code = SUCCESS;
            memcpy(server_rep.data, msg.data, msg.size);
            if ((r = writen(connFd, (void*)&server_rep, sizeof(server_rep))) < 0) {
                free(msg.data);
                perror("writen");
                break;
            }
            //free(msg.data);
        }
        else { //file_status == CLOSED - Il file non è stato aperto, non è stato possibile leggerlo
            printf("File is closed - FAILED\n");
            server_rep.reply_code = FAILED;
            if ((r = writen(connFd, (void*)&server_rep, sizeof(server_rep))) < 0) {
                free(msg.data);
                perror("writen");
                break;
            }
        }
        break;
        case READNFILES: {
        /*Richiede al server la lettura di ‘N’ files qualsiasi da memorizzare nella directory ‘dirname’ lato client. Se il server
ha meno di ‘N’ file disponibili, li invia tutti. Se N<=0 la richiesta al server è quella di leggere tutti i file
memorizzati al suo interno. Ritorna un valore maggiore o uguale a 0 in caso di successo (cioè ritorna il n. di file
effettivamente letti), -1 in caso di fallimento, errno viene settato opportunamente.
*/      int n_of_files;
        if ((r = readn(connFd, &n_of_files, sizeof(int))) < 0) {
            perror("readn");
            break;
        }
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

    //Scrivo sulla pipe di comunicazione col main thread il descrittore per segnalare che il worker ha finito
    if (writen(req_pipe, &connFd, sizeof(long)) == -1) {
		perror("write pipe");
		return;
	}
}




int main (int argc, char **argv) {
    initialize_table();
    if ((conf = read_config("../test/config.txt")) == NULL) {
        fprintf(stderr, "Errore nella lettura del file config.txt\n");
        exit(EXIT_FAILURE);
    }
    //icl_entry_t *entry;
    
    /*Debug inserimento file server*/    
    icl_hash_insert(hTable, "pippo", "prova contenuto");
    
    int num_of_threads_in_pool = conf->num_of_threads;
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
    // ignoro SIGPIPE per evitare di essere terminato da una scrittura su un socket
    struct sigaction s;
    memset(&s,0,sizeof(s));    
    s.sa_handler=SIG_IGN;
    if ( (sigaction(SIGPIPE,&s,NULL) ) == -1 ) {   
	perror("sigaction");
	goto _exit;
    } 

    /*
     * La pipe viene utilizzata come canale di comunicazione tra il signal handler thread ed il 
     * thread lisener per notificare la terminazione. 
     * Una alternativa è quella di utilizzare la chiamata di sistema 
     * 'signalfd' ma non e' POSIX.
     *
     */
    int signal_pipe[2];
    if (pipe(signal_pipe)==-1) {
		perror("signal pipe");
		goto _exit;
    	}

    //Pipe di notifica di una richiesta che è stata servita e il thread è di nuovo pronto per altre richieste
	int pipe_request[2];
	if (pipe(pipe_request) == -1) {
		perror("pipe request");
		goto _exit;
	}
    pthread_t sighandler_thread;
    sigHandler_t handlerArgs = { &mask, signal_pipe[1] };
   
    if (pthread_create(&sighandler_thread, NULL, sigHandler, &handlerArgs) != 0) {
		fprintf(stderr, "errore nella creazione del signal handler thread\n");
		goto _exit;
    }
    
    int listenfd;
    if ((listenfd= socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		goto _exit;
    }

    struct sockaddr_un serv_addr;
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;    
    strncpy(serv_addr.sun_path, conf->sock_name, strlen(conf->sock_name)+1);

    if (bind(listenfd, (struct sockaddr*)&serv_addr,sizeof(serv_addr)) == -1) {
	perror("bind");
	goto _exit;
    }
    if (listen(listenfd, MAXBACKLOG) == -1) {
	perror("listen");
	goto _exit;
    }

    threadpool_t *pool = NULL;

    pool   = createThreadPool(num_of_threads_in_pool, num_of_threads_in_pool); 
    if (!pool) {
		fprintf(stderr, "ERRORE FATALE NELLA CREAZIONE DEL THREAD POOL\n");
		goto _exit;
    }
    
    fd_set set, tmpset;
    FD_ZERO(&set);
    FD_ZERO(&tmpset);

    FD_SET(listenfd, &set);        
    FD_SET(signal_pipe[0], &set);  // descrittore di lettura della signal_pipe
    FD_SET(pipe_request[0], &set); // descrittore di lettura della pipe_request

    // tengo traccia del file descriptor con id piu' grande
    int fdmax = (listenfd > signal_pipe[0]) ? listenfd : signal_pipe[0];

    volatile long termina=0;
    while(!termina) {
	// copio il set nella variabile temporanea per la select
	tmpset = set;
	if (select(fdmax+1, &tmpset, NULL, NULL, NULL) == -1) {
	    perror("select");
	    goto _exit;
	}
	// cerchiamo di capire da quale fd abbiamo ricevuto una richiesta
	for(int i=0; i <= fdmax; i++) {
	    if (FD_ISSET(i, &tmpset)) {
		long connfd;
		if (i == listenfd) { // e' una nuova richiesta di connessione 
		    if ((connfd = accept(listenfd, (struct sockaddr*)NULL ,NULL)) == -1) {
			perror("accept");
			goto _exit;
		    }
			else
				printf("Client connesso\n");
			FD_SET(connfd, &set);
			if (connfd > fdmax)
				fdmax = connfd;
		}
		else if (i == pipe_request[0]) { //Il thread ha servito 
			int n;
			long desc;
			if ((n=readn(pipe_request[0], &desc, sizeof(long))) == -1) {
				perror("read");
				continue;
			}
			printf("ThreadWorker ha servito\n");
			FD_SET(desc, &set);
			if (desc > fdmax)
				fdmax = desc;
		}

		else if (i == signal_pipe[0]) {
		    // ricevuto un segnale, esco ed inizio il protocollo di terminazione
		    termina = 1;
		    break;
		}
		else { //
			long* args = malloc(3*sizeof(long));
		    if (!args) {
		      perror("FATAL ERROR 'malloc'");
		      goto _exit;
		    }
			args[0] = connfd;
		    //args[1] = (long)&termina;
			args[1] = (int) pipe_request[1];
			FD_CLR(i, &set);
			fdmax = updatemax(set, fdmax);
		    int r =addToThreadPool(pool, threadWorker, (void*)args);
		    if (r==0) continue; // aggiunto con successo
		    if (r<0) { // errore interno
			fprintf(stderr, "FATAL ERROR, adding to the thread pool\n");
		    } else { // coda dei pendenti piena
			fprintf(stderr, "SERVER TOO BUSY\n");
		    }
		    free(args);
		   	close(connfd);
		    continue;
		}
	    }
	}
    }
    
    destroyThreadPool(pool, 0);  // notifico che i thread dovranno uscire

    // aspetto la terminazione de signal handler thread
    pthread_join(sighandler_thread, NULL);

    unlink(conf->sock_name);    
    return 0;    
 _exit:
    unlink(conf->sock_name);
    return -1;
}
