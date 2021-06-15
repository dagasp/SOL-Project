#include "util.h"


config_file *read_config(char *config) {
    config_file *conf;
    FILE *fd = NULL;
    char *buf;
    long num;
    int line = 0;
    if ((fd=fopen(config), "r") == NULL) {
        fclose(fd);
        fprintf(stderr, "Apertura file - FATAL ERROR\n");
        return NULL;
    }
    conf = malloc(sizeof(config_file));
    if (!conf) {
        fclose(fd);
        fprintf(stderr, "Malloc - FATAL ERROR\n");
        return NULL;
    }
    buf = malloc(BUFSIZE * sizeof(char));
    if (!buf) {
        fclose(fd);
        fprintf(stderr, "Malloc - FATAL ERROR\n");
        return NULL;
    }
    while (fgets(buf, BUFSIZE, fd) != NULL) {
        switch (line) {
            case 0: 
                if (isNumber(buf, &num) == 0) {
                    conf->num_of_threads = (int*) num;
                }
                else {
                    fprintf("Errore nel config file, il parametro non è un numero!\n");
                    return NULL;
                }
                break;
            case 1:
                if (isNumber(buf, &num) == 0) {
                    conf->memory_space = num;
                }
                else {
                    fprintf("Errore nel config file, il parametro non è un numero!\n");
                    return NULL;
                }
                break;
            case 2:
                if (isNumber(buf, &num) == 0) {
                    fprintf(stderr, "Errore nel config file, il paramentro non è una stringa!\n");
                    return NULL;
                }
                strcpy(conf->sockname, buf);
                break;
            default:
                break;
        }
        line++;
    }
    fclose(fd);
    free(buf);
    return conf;
}

/** Evita letture parziali
 *
 *   \retval -1   errore (errno settato)
 *   \retval  0   se durante la lettura da fd leggo EOF
 *   \retval size se termina con successo
 */
static inline int readn(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char*)buf;
    while(left>0) {
	if ((r=read((int)fd ,bufptr,left)) == -1) {
	    if (errno == EINTR) continue;
	    return -1;
	}
	if (r == 0) return 0;   // EOF
        left    -= r;
	bufptr  += r;
    }
    return size;
}

/** Evita scritture parziali
 *
 *   \retval -1   errore (errno settato)
 *   \retval  0   se durante la scrittura la write ritorna 0
 *   \retval  1   se la scrittura termina con successo
 */
static inline int writen(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char*)buf;
    while(left>0) {
	if ((r=write((int)fd ,bufptr,left)) == -1) {
	    if (errno == EINTR) continue;
	    return -1;
	}
	if (r == 0) return 0;  
        left    -= r;
	bufptr  += r;
    }
    return 1;
}