#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hash.h"

int hash (const char *key, int n_buckets) { 
    unsigned long hash = 5381;
    int x;
    while (x = *key++)
        hash = ((hash << 5) + hash) + x;
    return hash % n_buckets;
}

int hash_key_cmpr(const char *key, const char *key1) {
    return strcmp(key, key1);
        /*<0	the first character that does not match has a lower value in ptr1 than in ptr2
        0	the contents of both strings are equal
        >0	the first character that does not match has a greater value in ptr1 than in ptr2*/
}

hashTable *table_initialize(int nbuckets) {
    hashTable *hT;
    hT = (hashTable*) malloc(sizeof(hashTable));
    if (!hT) {
        fprintf(stderr, "Malloc - FATAL ERROR\n");
        return NULL;
    }
    hT->n_of_entries = 0;
    hT->buckets = (node **)malloc(nbuckets * sizeof(node*));
    if (!hT->buckets) {
        fprintf(stderr, "Malloc - FATAL ERROR\n");
        return NULL;
    }
    hT->n_buckets = nbuckets;
    
    //initialize
    for (int i = 0; i < nbuckets; i++) {
        hT->buckets[i] = NULL;
    }
    return hT;
}

int search_for_key(hashTable *hT, const char *key) {
    unsigned int hash_index = hash(key, hT->n_buckets);
    while (hT->buckets[hash_index] != NULL) {
        if ((hash_key_cmpr(hT->buckets[hash_index]->path_name), key) == 0)
            return 1; //Found
        ++hash_index;
    }
    return 0; //Not Found
}

void insert_by_key(hashTable *ht, const char *key, const char *content) {
    //Indice in cui inserire
    unsigned int hash_index = hash(key, ht->n_buckets);
    node *curr;
    if (search_for_key(ht, key) == 1) {
        printf("Key already exists!\n");
        return NULL;
    }
    curr = (node*)malloc(sizeof(node));
    if (!curr) {
        fprintf(stderr, "Malloc - FATAL ERROR\n");
        return NULL;
    }
    strcpy(curr->path_name, key);
    strcpy(curr->content, content);
    curr->next = ht->buckets[hash_index];
    ht->buckets[hash_index] = curr;
    ht->n_of_entries++;
    free(curr);
}

void delete_by_key (hashTable *hT, const char *key) {
    unsigned int hash_index = hash(key, hT->n_buckets);
    while (hT->buckets[hash_index] != NULL) {
        if (hash_key_cmpr(hT->buckets[hash_index]->path_name, key) == 0) 
}