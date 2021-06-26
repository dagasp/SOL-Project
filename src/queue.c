#include "queue.h"

fqueue *create () {
    fqueue *queue = malloc(sizeof(fqueue));
    queue->head = NULL;
    queue->tail = NULL;
    return queue;
}

void insert (fqueue *queue, const char opcode, void *value) {
    node *tmp = malloc(sizeof(struct node));
    tmp->op_code = opcode;
    tmp->data = value;
    tmp->next = NULL;
    if (queue->head == NULL) { //testa vuota
        queue->head = tmp;
        queue->tail = tmp;
    }
    else { 
        node *old_tail = queue->tail;
        old_tail->next = tmp;
        queue->tail = tmp;
    }
}

node *pop(fqueue *queue) {
    if (queue == NULL)
        return NULL;
    node *n = (node*)queue->head;
    node *saved_node = queue->head;
    queue->head = queue->head->next;
    free(n);
    return saved_node;
}

void dequeue (fqueue *queue) {
    node *head = queue->head;
    queue->head = head->next;
    free (head);
}
