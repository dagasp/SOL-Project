#include "util.h"


config_file *read_config(char *config) {
    config_file *conf;
    FILE *fd = NULL;
    char *buf;
    //long num;
    int line = 0;
    if ((fd=fopen(config, "r")) == NULL) {
        //fclose(fd);
        fprintf(stderr, "Errore nell'apertura del file di config\n");
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
                /*if (isNumber(buf, &num) != 0) {
                    conf->num_of_threads = num;
                }
                else {
                    fprintf(stderr, "Errore nel config file, il parametro 'number of threads' non è un numero!\n");
                    return NULL;
                }*/
                conf->num_of_threads = atoi(buf);
                break;
            case 1: ;
               /* if (isNumber(buf, &num) == 0) {
                    conf->memory_space = num;
                }
                else {
                    fprintf(stderr, "Errore nel config file, il parametro 'memory space' non è un numero!\n");
                    return NULL;
                }*/
                conf->memory_space = atoi(buf);
                break;
            case 2:
                conf->num_of_files = atoi(buf);
                break;
            case 3: {
                char *tmp = buf;
                char *token = strtok(tmp, "\r\n"); //Serve a pulire la stringa togliendo lo /n o /r
                strcpy(conf->sock_name, token);
                //printf("%s\n", conf->sock_name);
                break;
            }
                /*if (isNumber(buf, &num) == 0) {
                    fprintf(stderr, "Errore nel config file, il parametro 'sockname' non è una stringa!\n");
                    return NULL;
                }*/
            case 4: {
                char *tmp = buf;
                char *token = strtok(tmp, "\r\n"); //Serve a pulire la stringa togliendo lo /n o /r
                strcpy(conf->log_file, token);
                break;
            }
            default: ;
                break;
        }
        line++;
    }
    fclose(fd);
    free(buf);
    return conf;
}


int check_for_dir (const char *directory) {
    DIR *dir = opendir(directory);
    if (!dir) return -1;
    return 0;
}

int mk_directory (const char *directory) {
    if (check_for_dir(directory) != 0)
        return mkdir(directory, 0777);
    else return -1;
}

char *get_path(const char *path) {
    return realpath(path, NULL);
}