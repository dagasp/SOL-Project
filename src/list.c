#include "list.h"

void put_by_key(list **head, const char *key, int desc) {
    list *tmp = malloc(sizeof(list));
    if (!tmp) {
        fprintf(stderr, "Errore nella malloc\n");
        return;
    }
    strncpy(tmp->pathname, key, MAX_PATH);
    tmp->descriptor = desc;
    tmp->next = *head;
    *head = tmp;
}

int delete_by_key(list **head, char *k, int desc) {
    list *tmp = *head;
    list *prev = NULL;
    if (*head == NULL || head == NULL) return 0;
    while (tmp != NULL) {
       // printf("QUEUE DATA: %s\n", (char*)tmp->data);
        //printf("KEY DATA: %s\n", (char*)k);
        if ((strncmp(tmp->pathname, k, MAX_PATH) == 0) && tmp->descriptor == desc) {
            if (prev == NULL) {
                *head = (*head)->next;
                free(tmp);
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
        if ((strncmp((char*)tmp->pathname, path, MAX_PATH) == 0) && tmp->descriptor == desc)
            return 0; //File found, aperto
        tmp = tmp->next; 
    }
    return -1;
}

int insert_file(list **head, char *path) {
    list *curr;
    for (curr=*head; curr != NULL; curr=curr->next)
        if (strncmp(curr->pathname, path, MAX_PATH) == 0)
            return -1; /* File esiste già */

    //File non esiste in lista
    list *tmp = malloc(sizeof(list));
    if (!tmp) {
        fprintf(stderr, "Errore nella malloc\n");
        return -1;
    }
    strncpy(tmp->pathname, path, MAX_PATH);
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
        printf("PATHNAME: %s\n", (char*)list->pathname);
        printf("ASSOCIATED SOCKET: %d\n", list->descriptor);
        list = list->next;
    }
}