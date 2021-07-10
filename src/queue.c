#include "queue.h"

fqueue *create () {
    fqueue *queue = malloc(sizeof(fqueue));
    queue->head = NULL;
    queue->tail = NULL;
    return queue;
}

void insert (fqueue *queue, unsigned int opcode, void *value) {
    node *tmp = malloc(sizeof(struct node));
    if (!tmp) {
        fprintf(stderr, "Errore nella malloc\n");
        return;
    }
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
    queue->head = queue->head->next;
    return n;
}

void dequeue (fqueue *queue) {
    node *head = queue->head;
    queue->head = head->next;
    free (head);
}

void delete_queue(fqueue *q) {
    while (q->head != q->tail) {
        node *to_free = q->head->data;
        q->head = q->head->next;
        free(to_free);
    }
    if (q->head) free (q->head);
    free(q);
}