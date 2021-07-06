#if !defined(_LIST_H)
#define _LIST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

typedef struct node {
    char pathname[PATH_MAX];
    int descriptor;
    int curr_size;
    struct node *next;
} node;

void init(node *list);

void put_by_key (node **head, const char *key, int desc);

int delete_by_key(node **head, char *k);

void print_q(node *list);

int list_contain_file(node *list, char *, int);


#endif