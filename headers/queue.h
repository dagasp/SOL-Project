#if !defined(_QUEUE_H)
#define _QUEUE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct node {
    unsigned int op_code;
    void *data;
    int descriptor;
    struct node *next;
} node;

typedef struct {
    struct node *head;
    struct node *tail;
} fqueue;

fqueue *create ();

void insert (fqueue *queue, unsigned int opcode, void *value);

node *pop(fqueue *queue);

void dequeue (fqueue *queue);

void delete_queue(fqueue *q);

#endif