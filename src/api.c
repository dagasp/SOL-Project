#include "api.h"

static long fd_skt;

/*Scrive il contenuto di un file sul disco. Se dirname non esiste, verrà creata.
Ritorna 0 in caso di successo, -1 in caso di errore.*/

int writeToFile(char *pathname, char *content, const char *dirname, size_t fileSize) {
    if (!pathname) {
        fprintf(stderr, "writeToFile: Pathname non valido\n");
        errno = EINVAL;
        return -1;
    }
    //printf("%ld\n", fileSize);
     //Controllo che la cartella esista
    char *tmp_dirname = get_path(dirname); //Prendo path assoluto, se esiste
    if (!tmp_dirname) {
        if (mk_directory(dirname) != 0) { //Cartella non esiste-> la creo
            fprintf(stderr, "Impossibile creare la cartella %s\n", dirname);
            return -1;
       }
       tmp_dirname = get_path(dirname);        
    }
    FILE *stream = NULL;
    strcat(tmp_dirname, "/");
    char *fileName = basename(pathname); //Prende il nome del file da pathname
    if ((stream = fopen(strcat(tmp_dirname, fileName), "wb")) == NULL) { 
            fprintf(stderr, "Errore nell'apertura del file\n");
            free(tmp_dirname);
            return -1;
    }
    //fprintf(stream, "%s", (char*)content);
    int err;
    if (fwrite(content, sizeof(char), fileSize, stream) < 0) {
        SYSCALL_RETURN("fclose", err, fclose(stream), "Impossibile chiudere il file\n", "");
        return -1;
    }
    if (fclose(stream) != 0) {
        fprintf(stderr, "Errore nella chiusura del file\n");
        free(tmp_dirname);
        return -1;
    }
    free(tmp_dirname);
    return 0;
}

/*Viene aperta una connessione AF_UNIX al socket file sockname. Se il server non accetta immediatamente la
richiesta di connessione, la connessione da parte del client viene ripetuta dopo ‘msec’ millisecondi e fino allo
scadere del tempo assoluto ‘abstime’ specificato come terzo argomento. Ritorna 0 in caso di successo, -1 in caso
di fallimento, errno viene settato opportunamente.*/

int openConnection(const char *sockname, int msec, const struct timespec abstime) {
    errno = 0;
    int r = 0; 
    if (!sockname) {
        fprintf(stderr, "openConnection: sockname non valido\n");
        errno = EINVAL;
        return -1;
    }
    struct sockaddr_un sa;
    memset(&sa, '0', sizeof(sa));
    strncpy(sa.sun_path, sockname, UNIX_MAX_PATH);
    sa.sun_family=AF_UNIX;
    SYSCALL_RETURN("socket", fd_skt,socket(AF_UNIX,SOCK_STREAM,0), "Errore nella creazione del socket\n", "");
    struct timespec current_time;
    SYSCALL_RETURN("clock_getime", r, clock_gettime(CLOCK_REALTIME, &current_time), "Errore in clock_gettime\n", "");
    while ((r = connect(fd_skt,(struct sockaddr*)&sa,sizeof(sa))) != 0 && abstime.tv_sec > current_time.tv_sec) {
        usleep(msec * 1000); //usleep for ms
        SYSCALL_RETURN("clock_getime", r, clock_gettime(CLOCK_REALTIME, &current_time), "Errore in clock_gettime\n", "");
    }
    errno = ETIMEDOUT;
    if (r != -1)
        return 0;
    return r;
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
    if (!pathname) {
        fprintf(stderr, "openFile: Pathname non valido\n");
        errno = EINVAL;
        return -1;
    }
    client_operations client_op;
    memset(&client_op, 0, sizeof(client_operations));
    char *realPath = get_path(pathname);
    if (!realPath) {
        fprintf(stderr, "openFile: Pathname non trovato\n");
        return -1;
    } 
    client_op.flags = flags;
    client_op.op_code = OPENFILE;
    client_op.client_desc = fd_skt;
    strcpy(client_op.pathname,realPath);
    int n, rep_code;
    SYSCALL_RETURN("writen", n, writen(fd_skt, (void*)&client_op, sizeof(client_operations)), "Errore nell'invio della richiesta di apertura file\n", "");
    SYSCALL_RETURN("readn", n, readn(fd_skt, &rep_code, sizeof(int)), "Errore, impossibile ricevere risposta dal Server\n", "");
    if (rep_code == FAILED) 
        return FAILED;
    return SUCCESS;
}

/*Legge tutto il contenuto del file dal server (se esiste) ritornando un puntatore ad un'area allocata sullo heap nel
parametro ‘buf’, mentre ‘size’ conterrà la dimensione del buffer dati (ossia la dimensione in bytes del file letto). In
caso di errore, ‘buf‘e ‘size’ non sono validi. Ritorna 0 in caso di successo, -1 in caso di fallimento, errno viene
settato opportunamente.
*/

int readFile(const char* pathname, void** buf, size_t* size) {
    if (!pathname) {
        fprintf(stderr, "readFile: Pathname non valido\n");
        errno = EINVAL;
        return -1;
    }
    int n;
    client_operations client_op;
    server_reply server_rep;
    memset(&client_op, 0, sizeof(client_operations));
    memset(&server_rep, 0, sizeof(server_reply));
    client_op.op_code = READFILE;
    client_op.client_desc = fd_skt;
    char *realPath = get_path(pathname);
    if (!realPath) {
        fprintf(stderr, "readFile: Pathname non trovato\n");
        return -1;
    } 
    memcpy(client_op.pathname, realPath, strlen(realPath)+1);
    SYSCALL_RETURN("writen", n, writen(fd_skt, (void*)&client_op, sizeof(client_operations)), "Errore nell'invio della richiesta di lettura\n", "");
    SYSCALL_RETURN("readn", n, readn(fd_skt, (void*)&server_rep, sizeof(server_reply)), "Errore - impossibile ricevere risposta dal server\n", "");
    if (server_rep.reply_code == FAILED) { //Il server ritorna un errore, non ha letto il file
        fprintf(stderr, "Errore - il server non è riuscito a leggere il file\n");
        return -1;
    } 
    else {
        memcpy(*buf, server_rep.data, server_rep.size+1);
        *size = server_rep.size;
    }
    return 0;
}

/*Richiede al server la lettura di ‘N’ files qualsiasi da memorizzare nella directory ‘dirname’ lato client. Se il server
ha meno di ‘N’ file disponibili, li invia tutti. Se N<=0 la richiesta al server è quella di leggere tutti i file
memorizzati al suo interno. Ritorna un valore maggiore o uguale a 0 in caso di successo (cioè ritorna il n. di file
effettivamente letti), -1 in caso di fallimento, errno viene settato opportunamente.
*/
int readNFiles(int N, const char *dirname) { //Controllare se dirname = NULL, in quel caso non memorizzare su disco
    client_operations client_op;
    server_reply server_rep;
    memset(&client_op, 0, sizeof(client_operations));
    memset(&server_rep, 0, sizeof(server_reply));
    client_op.op_code = READNFILES;
    client_op.files_to_read = N;
    if (dirname) { //Ho ricevuto -d, mi preparo per scrivere su file in locale
        strcpy(client_op.dirname, dirname);
    }
        
    int files_letti = 0;
    int n;
    SYSCALL_RETURN("writen", n, writen(fd_skt, (void*)&client_op, sizeof(client_operations)), "Impossibile inviare richiesta di leggere N files al server\n", "");
    SYSCALL_RETURN("readn", n, readn(fd_skt, &files_letti, sizeof(int)), "Errore - impossibile ricevere risposta dal server\n", ""); 
    for (int i = 0; i < files_letti; i++) {
        SYSCALL_RETURN("readn", n, readn(fd_skt, &server_rep, sizeof(server_reply)), "Errore - impossibile ricevere risposta dal server\n", "");
        if (dirname) { //Se ho ricevuto il comando -d scrivo su file
            if (writeToFile(server_rep.pathname, server_rep.data, dirname, server_rep.size) != 0) {
                fprintf(stderr, "Errore nella scrittura dei files\n");
                return -1;
            } 
        }  
        memset(&server_rep, 0, sizeof(server_reply));
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
    if (!pathname) {
        fprintf(stderr, "appendToFile: Pathname non valido\n");
        errno = EINVAL;
        return -1;
    }
    if (!buf) {
        fprintf(stderr, "appendToFile: Contenuto da appendere non valido\n");
        errno = EINVAL;
        return -1;
    }
    client_operations client_op;
    server_reply server_rep;
    memset(&client_op, 0, sizeof(client_operations));
    memset(&server_rep, 0, sizeof(server_reply));
    char *realPath = get_path(pathname);
    if (!realPath) {
        fprintf(stderr, "appendToFile: Pathname non trovato\n");
        return -1;
    } 
    client_op.op_code = APPENDTOFILE;
    client_op.size = size;
    client_op.client_desc = fd_skt;
    memcpy(client_op.pathname, realPath, strlen(realPath)+1);
    memcpy(client_op.data, buf, size);
    int n, fb;
    SYSCALL_RETURN("writen", n, writen(fd_skt, (void*)&client_op, sizeof(client_operations)), "Impossibile inviare richiesta di append al server\n", "");
    SYSCALL_RETURN("readn", n, readn(fd_skt, &fb, sizeof(int)), "Impossibile ricevere risposta dal server\n", "");
    return fb;
}

/*Richiesta di chiusura del file puntato da ‘pathname’. Eventuali operazioni sul file dopo la closeFile falliscono.
Ritorna 0 in caso di successo, -1 in caso di fallimento, errno viene settato opportunamente.
*/
int closeFile(const char *pathname) {
    if (!pathname) {
        fprintf(stderr, "closeFile: Pathname non valido\n");
        errno = EINVAL;
        return -1;
    }
    client_operations client_op;
    memset(&client_op, 0, sizeof(client_operations));
    client_op.op_code = CLOSEFILE;
    client_op.client_desc = fd_skt;
    char *realPath = get_path(pathname);
    if (!realPath) {
        fprintf(stderr, "closeFile: Pathname non trovato\n");
        return -1;
    } 
    strcpy(client_op.pathname, realPath);
    int n, fb;
    SYSCALL_RETURN("writen", n, writen(fd_skt, (void*)&client_op, sizeof(client_operations)), "Impossibile inviare richiesta di chiusura del file al server\n", "");
    SYSCALL_RETURN("readn", n, readn(fd_skt, &fb, sizeof(int)), "Impossibile ricevere risposta dal server\n", "");
    return fb;
}

/*Chiude la connessione AF_UNIX associata al socket file sockname. Ritorna 0 in caso di successo, -1 in caso di
fallimento, errno viene settato opportunamente.*/

int closeConnection(const char *sockname) {
    if (!sockname) {
        fprintf(stderr, "closeConnection: Pathname non valido\n");
        errno = EINVAL;
        return -1;
    }
    int n;
    SYSCALL_RETURN("close", n, close(fd_skt), "Impossibile chiudere il socket associato\n", "");
    return 0;
}


/*Legge un file in locale e manda al server pathname e contenuto
Ritorna 0 in caso di successo, -1 in caso di errore.
errno viene settato */
int writeFile (char *file_name) {
    FILE *fp = NULL;
    if ((fp = fopen(file_name, "rb")) == NULL) {
        fprintf(stderr, "Errore nell'apertura del file\n");
        return -1;
    }
    char *path = get_path(file_name);
    if (!path) {
        fprintf(stderr, "writeFile: Pathname non trovato\n");
        return -1;
    } 
    client_operations client_op;
    memset(&client_op, 0, sizeof(client_operations));
    char *to_read = malloc(sizeof(char)*MAX_FILE_SIZE);
    if (!to_read) {
        fprintf(stderr, "Errore nella malloc\n");
        errno = ENOMEM;
        return -1;
    }
    memset(to_read, 0, MAX_FILE_SIZE);
    size_t n_read = fread(to_read, sizeof(char), MAX_FILE_SIZE, fp);
    int n;
    if (n_read < 0) {
        fprintf(stderr, "Impossibile leggere dal file\n");
        SYSCALL_EXIT("fclose", n, fclose(fp), "Impossibile chiudere il file\n", "");
    }
    //printf("%ld\n", n_read);
    SYSCALL_EXIT("fclose", n, fclose(fp), "Impossibile chiudere il file\n", "");
    client_op.size = n_read;
    client_op.op_code = WRITEFILE;
    strcpy(client_op.pathname, path);
    memcpy(client_op.data, to_read, n_read);
    free(to_read);
    int fb;
    SYSCALL_RETURN("writen", n, writen(fd_skt, (void*)&client_op, sizeof(client_operations)), "Impossibile inviare richiesta di write al server\n", "");
    SYSCALL_RETURN("readn", n, readn(fd_skt, &fb, sizeof(int)), "Impossibile ricevere risposta dal server\n", "");
    return fb;
}