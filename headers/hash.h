#if !defined(_HASH_H)
#define _HASH_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

typedef struct node {
    const char *path_name; //key
    const char *content;
    struct node *next;
} node;

typedef struct hashTable {
    int n_of_entries;
    int n_buckets;
    node **buckets;
} hashTable;


int hash (const char *key, int n_buckets);

int hash_key_cmpr(const char *key, const char *key1);

hashTable *table_initialize(int nbuckets);

int search_for_key(hashTable *hT, const char *key);

void insert_by_key(hashTable *ht, const char *key, const char *content);

void delete_by_key (hashTable *hT, const char *key);

#endif