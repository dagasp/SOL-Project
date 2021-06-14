#if !defined(_LIST_H)
#define _LIST_H

typedef struct node {
    const char *path_name;
    const char *content;
    struct node *next;
} node;


#endif