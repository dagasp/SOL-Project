#include "api.h"
#include "util.h"
#include "hash.h"

static int fd_skt;

int openConnection(const char *sockname, int msec, const struct timespec abstime) {
    errno = 0;
    int r = 0; 
    struct sockaddr_un sa;
    memset(&sa, '0', sizeof(sa));
    strncpy(sa.sun_path, sockname,UNIX_PATH_MAX);
    sa.sun_family=AF_UNIX;
    SYSCALL_RETURN("socket", fd_skt,socket(AF_UNIX,SOCK_STREAM,0), "Errore nella creazione del socket\n");
    const struct timespec current_time;
    SYSCALL_RETURN("clock_getime, "r, clock_gettime(CLOCK_REALTIME, &current_time), "Errore in clock_gettime\n");
    while (r = connect(fd_skt,(struct sockaddr*)&sa,sizeof(sa)) != 0 && abstime.tv_sec > current_time.tv_sec) {
        usleep(msec * 1000);
        r = 0;
        SYSCALL_RETURN("clock_getime, "r, clock_gettime(CLOCK_REALTIME, &current_time), "Errore in clock_gettime\n");
    }
    if (r != -1)
        return 0;
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
    client_operations *client_op;
    client_op->flags = flags;
    client_op->op_code = OPENFILE;
    int n, rep_code;
    SYSCALL_RETURN("writen", n, writen(fd_skt, client_op, sizeof(client_op)), "Errore nell'invio della richiesta di apertura file\n");
    SYSCALL_RETURN("readn", n, readn(fd_skt, &rep_code, sizeof(int)), "Errore, impossibile ricevere risposta dal Server\n");
    //da settare errno
}

/*Legge tutto il contenuto del file dal server (se esiste) ritornando un puntatore ad un'area allocata sullo heap nel
parametro ‘buf’, mentre ‘size’ conterrà la dimensione del buffer dati (ossia la dimensione in bytes del file letto). In
caso di errore, ‘buf‘e ‘size’ non sono validi. Ritorna 0 in caso di successo, -1 in caso di fallimento, errno viene
settato opportunamente.
*/

int readFile(const char* pathname, void** buf, size_t* size) {
    int n;
    client_operations *client_op;
    server_reply *server_rep;
    client_op->op_code = READFILE;
    strcpy(client_op->pathname, pathname);
    SYSCALL_RETURN("writen", n, writen(fd_skt, client_op, sizeof(client_op)), "Errore nell'invio della richiesta di lettura\n", "");
    SYSCALL_RETURN("readn", n, readn(fd_skt, server_rep, sizeof(server_rep)), "Errore - impossibile ricevere risposta dal server\n", "");
    if (server_rep->reply_code == -1) { //Il server ritorna un errore, non ha letto il file
        fprintf(stderr, "Errore - il server non è riuscito a leggere il file\n");
        return -1;
    } 
    else {
        strcpy(*buf, server_rep->data);
        size = strlen(*buf);
    }
    //settare errno
}

/*Richiede al server la lettura di ‘N’ files qualsiasi da memorizzare nella directory ‘dirname’ lato client. Se il server
ha meno di ‘N’ file disponibili, li invia tutti. Se N<=0 la richiesta al server è quella di leggere tutti i file
memorizzati al suo interno. Ritorna un valore maggiore o uguale a 0 in caso di successo (cioè ritorna il n. di file
effettivamente letti), -1 in caso di fallimento, errno viene settato opportunamente.
*/
int readNFiles(int N, const char *dirname) {
    client_operations *client_op;
    server_reply *server_rep;
    client_op->op_code = READFILE;
    int files_letti = 0;
    int n;
    SYSCALL_RETURN("writen", n, writen(fd_skt, N, sizeof(int)), "Impossibile inviare richiesta al server\n", "");
    SYSCALL_RETURN("readn", n, readn(fd_skt, server_rep, sizeof(server_rep)), "Errore - impossibile ricevere risposta dal server\n", "");
    FILE *fp;
    files_letti = server_rep->n_files_letti;
    for (int i = 0; i < files_letti; i++) { //supponendo cartella esista, da implementare se cartella non esiste
        char *path = strcat(dirname, server_rep->pathname); //da estrarre path assoluto
        fp = fopen(path, "w+"); 
    }

}

