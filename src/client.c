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
    printf("Usage: %s -> ", program_name); //inserire comandi supportati
}

fqueue queue = create();

void send_request () {
    int err_code;
    node *n;
    n = pop(queue);
    int opt = n->op_code;
    switch (opt) {
        case 'r':
            
            break;
    }
}

int main(int argc, char **argv) {
    char *data = NULL;
    long n_files = 0;
     if (argc == 1) {
	    printf("Nessun argomento passato al programma\n");
	    exit(EXIT_FAILURE);
        }
    int opt;
    //Da implementare: -h, -f, -r, -R, -t, -p
    while ((opt = getopt(argc, argv), "h:f:w:W:D:r:R:d:t:l:u:c:p") != -1) { 
        switch(opt) {
            case 'h':
                print_usage(argv[0]);
                return 0;
            case 'f':
                //da implementare
                break;
            case 'r':
                data = optarg;
                insert(queue, 'r', (void*)data);
                //inserire richiesta in lista -
                break;
            case 'R':
                n_files = isNumber(optarg, &n_files);
                insert(queue, 'R', (void*)n_files);
                break;
            default: 
                printf("Opzione non supportata\n");
                break;
        }
    }
    return 0;
}