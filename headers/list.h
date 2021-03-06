#if !defined(_LIST_H)
#define _LIST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

enum STATUS {
  OPEN = 0, 
  CLOSED = -1
};

typedef struct node {
    char pathname[MAX_PATH];
    long descriptor;
    int curr_size;
    int file_status;
    struct node *next;
} list;

void init(list *list);

void put_by_key (list **head, const char *key, long desc);
int insert_file(list **head, char *path);
char *get_last_file(list *l);
int delete_last_element(list **head);
int delete_by_key(list **head, char *k, long);
void print_q(list *list);
void list_destroy(list **head);
int list_contain_file(list *list, char *, int);


#endif