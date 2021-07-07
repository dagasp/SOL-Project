#include "list.h"

void put_by_key(list **head, const char *key, int desc) {
    list *tmp = malloc(sizeof(list));
    if (!tmp) {
        fprintf(stderr, "Errore nella malloc\n");
        return;
    }
    strncpy(tmp->pathname, key, PATH_MAX);
    tmp->descriptor = desc;
    tmp->next = *head;
    *head = tmp;
}

int delete_by_key(list **head, char *k) {
    list *tmp = *head;
    list *prev = NULL;
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

int list_contain_file(list *l, char *path, int desc) {
    list *tmp = l;
    while (tmp != NULL) {
        if (tmp->pathname != NULL)
        if ((strncmp((char*)tmp->pathname, path, PATH_MAX) == 0) && tmp->descriptor == desc)
            return 0; //File found, aperto
        tmp = tmp->next; 
    }
    return -1;
}

int insert_file(list **head, char *path) {
    list *curr;
    for (curr=*head; curr != NULL; curr=curr->next)
        if (strncmp(curr->pathname, path, PATH_MAX) == 0)
            return -1; /* File esiste già */

    //File non esiste in lista
    list *tmp = malloc(sizeof(list));
    if (!tmp) {
        fprintf(stderr, "Errore nella malloc\n");
        return -1;
    }
    strncpy(tmp->pathname, path, PATH_MAX);
    tmp->next = *head;
    *head = tmp;
    return 0;
}

char *get_last_file(list *l) {
    list *curr = l;
    list *next = curr->next;
    while (next != NULL) {
        curr = next;
        next = curr->next;
    }
    if (curr->pathname)  
        return curr->pathname;
    else return NULL;
}

void delete_last_element(list **head) {
    list *curr = *head;
    list *prev = NULL;
    while (curr->next != NULL) {
        prev = curr;
        curr = curr->next;
    }
    prev->next = NULL;
    free(curr);
}

void print_q(list *list) {
    while (list != NULL) {
        printf("QUEUE DATA: %s\n", (char*)list->pathname);
        list = list->next;
    }
}
