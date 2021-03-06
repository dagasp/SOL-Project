/**
 * @file
 *
 * Header file for icl_hash routines.
 *
 */
/* $Id$ */
/* $UTK_Copyright: $ */

#ifndef icl_hash_h
#define icl_hash_h
#define _GNU_SOURCE  
#include <stdio.h>
#include <pthread.h>
#include "util.h"
#include "list.h"

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif


/*
* @key -- file's pathname
* @data -- file's content
* @fileSize -- file's size in byte
* @next -- pointer to the next element
*/
typedef struct icl_entry_s {
    void* key;
    void *data;
    size_t fileSize;
    struct icl_entry_s* next;
} icl_entry_t;


/*
* @nbuckets -- number of buckets
* @nentries -- current number of entries
* @max_size -- maximum memory size
* @curr_size -- current size of the table
* @max_files -- maximum number of files
* @tableLock -- hash table's lock
*/
typedef struct icl_hash_s {
    int nbuckets;
    int nentries;
    size_t max_size;
    size_t curr_size;
    long max_files;
    icl_entry_t **buckets;
    pthread_mutex_t tableLock;
    unsigned int (*hash_function)(void*);
    int (*hash_key_compare)(void*, void*);
} icl_hash_t;

icl_hash_t *
icl_hash_create( int nbuckets, unsigned int (*hash_function)(void*), int (*hash_key_compare)(void*, void*), size_t s, long max);

void
* icl_hash_find(icl_hash_t *, void* key);

size_t
icl_hash_get_file_size(icl_hash_t *ht, void* key);

int 
icl_hash_find_and_append(icl_hash_t *ht, void* key, void *data_to_append);

icl_entry_t
* icl_hash_insert(icl_hash_t *, void*, void *, size_t);
size_t get_current_size (icl_hash_t *ht);

int
icl_hash_destroy(icl_hash_t *, void (*)(void*), void (*)(void*)),
    icl_hash_dump(long, icl_hash_t *ht, int);

int
icl_hash_dump_2(FILE* stream, icl_hash_t* ht);
int icl_hash_delete( icl_hash_t *ht, void* key, void (*free_key)(void*), void (*free_data)(void*) );

/* simple hash function */
unsigned int
hash_pjw(void* key);

/* compare function */
int 
string_compare(void* a, void* b);


int get_n_entries (icl_hash_t *);

int append (icl_hash_t *ht, void *key, char *new_data, size_t size);

#define icl_hash_foreach(ht, tmpint, tmpent, kp, dp)    \
    for (tmpint=0;tmpint<ht->nbuckets; tmpint++)        \
        for (tmpent=ht->buckets[tmpint];                                \
             tmpent!=NULL&&((kp=tmpent->key)!=NULL)&&((dp=tmpent->data)!=NULL); \
             tmpent=tmpent->next) \
              


#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

#endif /* icl_hash_h */
