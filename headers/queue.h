#include <stdio.h>
#include <stdlib.h>

typedef struct node {
    unsigned int op_code;
    void *data;
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