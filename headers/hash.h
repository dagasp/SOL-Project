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

#include <stdio.h>
#include "util.h"
#include "api.h"
#include "list.h"

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

typedef struct icl_entry_s {
    void* key;
    void *data;
    int modified; //File modificato - 0-> SI, 1 -> NO
    struct icl_entry_s* next;
} icl_entry_t;

typedef struct icl_hash_s {
    int nbuckets;
    int nentries;
    size_t max_size;
    size_t curr_size;
    long max_files;
    icl_entry_t **buckets;
    unsigned int (*hash_function)(void*);
    int (*hash_key_compare)(void*, void*);
} icl_hash_t;

icl_hash_t *
icl_hash_create( int nbuckets, unsigned int (*hash_function)(void*), int (*hash_key_compare)(void*, void*), size_t s, long max);

void
* icl_hash_find(icl_hash_t *, void* key);

icl_entry_t
* icl_hash_insert(icl_hash_t *, void*, void *),
    * icl_hash_update_insert(icl_hash_t *, void*, void *, void**);

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

/* Funzioni aggiunte*/
int get_n_entries (icl_hash_t *);



int append (icl_hash_t *ht, void *key, char *new_data, size_t size);

/*Fine funzioni aggiunte*/

//void get_file(icl_hash_t *t, char pathname[][], char content[][]);

#define icl_hash_foreach(ht, tmpint, tmpent, kp, dp)    \
    for (tmpint=0;tmpint<ht->nbuckets; tmpint++)        \
        for (tmpent=ht->buckets[tmpint];                                \
             tmpent!=NULL&&((kp=tmpent->key)!=NULL)&&((dp=tmpent->data)!=NULL); \
             tmpent=tmpent->next) \
              


#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

#endif /* icl_hash_h */
