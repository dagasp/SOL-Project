#include "list.h"

void put_by_key(node **head, const char *key, int desc) {
    node *tmp = malloc(sizeof(struct node));
    if (!tmp) {
        fprintf(stderr, "Errore nella malloc\n");
        return;
    }
    strncpy(tmp->pathname, key, PATH_MAX);
    tmp->descriptor = desc;
    tmp->next = *head;
    *head = tmp;
}

int delete_by_key(node **head, char *k) {
    node *tmp = *head;
    node *prev = NULL;
    while (tmp != NULL) {
       // printf("QUEUE DATA: %s\n", (char*)tmp->data);
        //printf("KEY DATA: %s\n", (char*)k);
        if (strncmp(tmp->pathname, k, PATH_MAX) == 0) {
            if (prev == NULL) {
                free(tmp);
                *head = NULL;
                return 0;
            }
            else {
                prev->next = tmp->next;
                free(tmp);
                return 0;
            }
        }
        prev = tmp;
        tmp = tmp->next;
    }
    return -1;
}

int list_contain_file(node *list, char *path, int desc) {
    node *tmp = list;
    while (tmp != NULL) {
        if (tmp->pathname != NULL)
        if ((strncmp((char*)tmp->pathname, path, PATH_MAX) == 0) && tmp->descriptor == desc)
            return 0; //File found, aperto
        tmp = tmp->next; 
    }
    return -1;
}

void print_q(node *list) {
    while (list != NULL) {
        printf("QUEUE DATA: %s\n", (char*)list->pathname);
        list = list->next;
    }
}
