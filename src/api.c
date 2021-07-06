#include "api.h"

static int fd_skt;

/*Scrive il contenuto di un file sul disco. Se dirname non esiste, verrà creata.
Ritorna 0 in caso di successo, -1 in caso di errore.*/

int writeToFile(char *pathname, char *content, const char *dirname) {
    //char *tmp_dirname = calloc(sizeof(char), PATH_MAX);
     //Controllo che la cartella esista
    char *tmp_dirname = get_path(dirname); //Prendo path assoluto, se esiste
    if (!tmp_dirname) {
        if (mk_directory(dirname) != 0) { //Cartella non esiste-> la creo
            fprintf(stderr, "Impossibile creare la cartella %s\n", dirname);
            return -1;
       }
       tmp_dirname = get_path(dirname);
       //strcpy(tmp_dirname, dirname);        
    }
    printf("%s\n", tmp_dirname);
    FILE *stream = NULL;
    strcat(tmp_dirname, "/");
    char *fileName = basename(pathname); //Prende il nome del file da pathname
    //printf("RET: %s\n", ret);
    if ((stream = fopen(strcat(tmp_dirname, fileName), "w")) == NULL) { 
            fprintf(stderr, "Errore nell'apertura del file\n");
            free(tmp_dirname);
            return -1;
    }
    fprintf(stream, "%s", (char*)content);
    if (fclose(stream) != 0) {
        fprintf(stderr, "Errore nella chiusura del file\n");
        free(tmp_dirname);
        return -1;
    }
    free(tmp_dirname);
    return 0;
}

int openConnection(const char *sockname, int msec, const struct timespec abstime) {
    errno = 0;
    int r = 0; 
    struct sockaddr_un sa;
    memset(&sa, '0', sizeof(sa));
    strncpy(sa.sun_path, sockname, UNIX_PATH_MAX);
    sa.sun_family=AF_UNIX;
    SYSCALL_RETURN("socket", fd_skt,socket(AF_UNIX,SOCK_STREAM,0), "Errore nella creazione del socket\n", "");
    struct timespec current_time;
    SYSCALL_RETURN("clock_getime", r, clock_gettime(CLOCK_REALTIME, &current_time), "Errore in clock_gettime\n", "");
    while ((r = connect(fd_skt,(struct sockaddr*)&sa,sizeof(sa))) != 0 && abstime.tv_sec > current_time.tv_sec) {
        sleep(msec * 1000);
        SYSCALL_RETURN("clock_getime", r, clock_gettime(CLOCK_REALTIME, &current_time), "Errore in clock_gettime\n", "");
    }
    if (r != -1)
        return 0;
    return r;
    //settare errno;
}

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

int openFile(const char *pathname, int flags) {
    client_operations client_op;
    memset(&client_op, 0, sizeof(client_op));
    client_op.flags = flags;
    client_op.op_code = OPENFILE;
    client_op.client_desc = fd_skt;
    strcpy(client_op.pathname,pathname);
    int n, rep_code;
    SYSCALL_RETURN("writen", n, writen(fd_skt, &client_op, sizeof(client_op)), "Errore nell'invio della richiesta di apertura file\n", "");
    SYSCALL_RETURN("readn", n, readn(fd_skt, &rep_code, sizeof(int)), "Errore, impossibile ricevere risposta dal Server\n", "");
    if (rep_code == FAILED) 
        return FAILED;
    return SUCCESS;
    //da settare errno
}

/*Legge tutto il contenuto del file dal server (se esiste) ritornando un puntatore ad un'area allocata sullo heap nel
parametro ‘buf’, mentre ‘size’ conterrà la dimensione del buffer dati (ossia la dimensione in bytes del file letto). In
caso di errore, ‘buf‘e ‘size’ non sono validi. Ritorna 0 in caso di successo, -1 in caso di fallimento, errno viene
settato opportunamente.
*/

int readFile(const char* pathname, void** buf, size_t* size) {
    int n;
    client_operations client_op;
    server_reply server_rep;
    memset(&client_op, 0, sizeof(client_op));
    memset(&server_rep, 0, sizeof(server_rep));
    client_op.op_code = READFILE;
    client_op.client_desc = fd_skt;
    //get_path(pathname);
    memcpy(client_op.pathname, pathname, strlen(pathname)+1);
    //strcpy(client_op.pathname, pathname);
    SYSCALL_RETURN("writen", n, writen(fd_skt, (void*)&client_op, sizeof(client_op)), "Errore nell'invio della richiesta di lettura\n", "");
    //Farsi mandare dal server la strlen del buffer da scrivere ---
    SYSCALL_RETURN("readn", n, readn(fd_skt, (void*)&server_rep, sizeof(server_rep)), "Errore - impossibile ricevere risposta dal server\n", "");
    if (server_rep.reply_code == FAILED) { //Il server ritorna un errore, non ha letto il file
        fprintf(stderr, "Errore - il server non è riuscito a leggere il file\n");
        return -1;
    } 
    else {
        memcpy(*buf, server_rep.data, strlen(server_rep.data)+1);
        *size = strlen(*buf);
    }
    return 0;
    //settare errno e occuparsi di questo -> In caso di errore, ‘buf‘e ‘size’ non sono validi
}

/*Richiede al server la lettura di ‘N’ files qualsiasi da memorizzare nella directory ‘dirname’ lato client. Se il server
ha meno di ‘N’ file disponibili, li invia tutti. Se N<=0 la richiesta al server è quella di leggere tutti i file
memorizzati al suo interno. Ritorna un valore maggiore o uguale a 0 in caso di successo (cioè ritorna il n. di file
effettivamente letti), -1 in caso di fallimento, errno viene settato opportunamente.
*/
int readNFiles(int N, const char *dirname) { //Controllare se dirname = NULL, in quel caso non memorizzare su disco
    client_operations client_op;
    server_reply server_rep;
    memset(&client_op, 0, sizeof(client_op));
    memset(&server_rep, 0, sizeof(server_rep));
    client_op.op_code = READNFILES;
    client_op.files_to_read = N;
    if (dirname)
        strcpy(client_op.dirname, dirname);
    int files_letti = 0;
    int n;
    //printf("Sono dentro l'API readNFiles\n");
    SYSCALL_RETURN("writen", n, writen(fd_skt, (void*)&client_op, sizeof(client_op)), "Impossibile inviare richiesta di leggere N files al server\n", "");
    SYSCALL_RETURN("readn", n, readn(fd_skt, &files_letti, sizeof(int)), "Errore - impossibile ricevere risposta dal server\n", ""); 
    printf("FILE DA LEGGERE: %d\n", files_letti);
    if (!dirname) {
        printf("Ciao\n"); //Non ho ricevuto comando -d, non devo scrivere su file
        return files_letti;
    }
    for (int i = 0; i < files_letti; i++) {
        SYSCALL_RETURN("readn", n, readn(fd_skt, &server_rep, sizeof(server_rep)), "Errore - impossibile ricevere risposta dal server\n", "");
        //printf("PATH RICEVUTO: %s\n", server_rep.pathname);
        //printf("CONTENT RICEVUTO: %s\n", server_rep.data);
        if (writeToFile(server_rep.pathname, server_rep.data, dirname) != 0) {
            fprintf(stderr, "Errore nella scrittura dei files\n");
            return -1;
        }
        memset(&server_rep, 0, sizeof(server_rep));
    }
    if (files_letti >= 0)
        return files_letti;
    else
        return -1;
}

/*Richiesta di scrivere in append al file ‘pathname‘ i ‘size‘ bytes contenuti nel buffer ‘buf’. L’operazione di append
nel file è garantita essere atomica dal file server. Se ‘dirname’ è diverso da NULL, il file eventualmente spedito
dal server perchè espulso dalla cache per far posto ai nuovi dati di ‘pathname’ dovrà essere scritto in ‘dirname’;
Ritorna 0 in caso di successo, -1 in caso di fallimento, errno viene settato opportunamente.
*/

int appendToFile(const char *pathname, void *buf, size_t size, const char *dirname) {
    client_operations client_op;
    memset(&client_op, 0, sizeof(client_op));
    client_op.op_code = APPENDTOFILE;
    client_op.size = size;
    strcpy(client_op.pathname, pathname);
    memcpy(client_op.data, buf, size);
    int n, fb;
    SYSCALL_RETURN("writen", n, writen(fd_skt, (void*)&client_op, sizeof(client_op)), "Impossibile inviare richiesta di append al server\n", "");
    SYSCALL_RETURN("readn", n, readn(fd_skt, &fb, sizeof(int)), "Impossibile ricevere risposta dal server\n", "");
    return fb;
}

/*Richiesta di chiusura del file puntato da ‘pathname’. Eventuali operazioni sul file dopo la closeFile falliscono.
Ritorna 0 in caso di successo, -1 in caso di fallimento, errno viene settato opportunamente.
*/
int closeFile(const char *pathname) {
    client_operations client_op;
    memset(&client_op, 0, sizeof(client_op));
    client_op.op_code = CLOSEFILE;
    strcpy(client_op.pathname, pathname);
    int n, fb;
    SYSCALL_RETURN("writen", n, writen(fd_skt, (void*)&client_op, sizeof(client_op)), "Impossibile inviare richiesta di chiusura del file al server\n", "");
    SYSCALL_RETURN("readn", n, readn(fd_skt, &fb, sizeof(int)), "Impossibile ricevere risposta dal server\n", "");
    return fb;
}

/*Chiude la connessione AF_UNIX associata al socket file sockname. Ritorna 0 in caso di successo, -1 in caso di
fallimento, errno viene settato opportunamente.*/

int closeConnection(const char *sockname) {
    //client_operations client_op;
    //memset(&client_op, 0, sizeof(client_op));
    //client_op.op_code = CLOSECONNECTION;
    int n;
    //SYSCALL_RETURN("writen", n, writen(fd_skt, (void*)&client_op, sizeof(client_op)), "Impossibile inviare richiesta al server\n", "");
    //SYSCALL_RETURN("readn", n, readn(fd_skt, &feedback, sizeof(int)), "Impossibile ricevere feedback dal server\n", "");
    SYSCALL_RETURN("close", n, close(fd_skt), "Impossibile chiudere il socket associato\n", "");
    return 0;
}

