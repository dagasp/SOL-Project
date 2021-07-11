/**
 * @file icl_hash.c
 *
 * Dependency free hash table implementation.
 *
 * This simple hash table implementation should be easy to drop into
 * any other peice of code, it does not depend on anything else :-)
 * 
 * @author Jakub Kurzak
 */
/* $Id: icl_hash.c 2838 2011-11-22 04:25:02Z mfaverge $ */
/* $UTK_Copyright: $ */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "hash.h"


#include <limits.h>


#define BITS_IN_int     ( sizeof(int) * CHAR_BIT )
#define THREE_QUARTERS  ((int) ((BITS_IN_int * 3) / 4))
#define ONE_EIGHTH      ((int) (BITS_IN_int / 8))
#define HIGH_BITS       ( ~((unsigned int)(~0) >> ONE_EIGHTH ))
/**
 * A simple string hash.
 *
 * An adaptation of Peter Weinberger's (PJW) generic hashing
 * algorithm based on Allen Holub's version. Accepts a pointer
 * to a datum to be hashed and returns an unsigned integer.
 * From: Keith Seymour's proxy library code
 *
 * @param[in] key -- the string to be hashed
 *
 * @returns the hash index
 */
unsigned int
hash_pjw(void* key)
{
    char *datum = (char *)key;
    unsigned int hash_value, i;

    if(!datum) return 0;

    for (hash_value = 0; *datum; ++datum) {
        hash_value = (hash_value << ONE_EIGHTH) + *datum;
        if ((i = hash_value & HIGH_BITS) != 0)
            hash_value = (hash_value ^ (i >> THREE_QUARTERS)) & ~HIGH_BITS;
    }
    return (hash_value);
}

int string_compare(void* a, void* b) 
{
    return (strcmp( (char*)a, (char*)b ) == 0);
}


int get_n_entries(icl_hash_t *t) {
    return (t->nentries);
}


/**
 * Create a new hash table.
 *
 * @param[in] nbuckets -- number of buckets to create
 * @param[in] hash_function -- pointer to the hashing function to be used
 * @param[in] hash_key_compare -- pointer to the hash key comparison function to be used
 *
 * @returns pointer to new hash table.
 */

icl_hash_t *
icl_hash_create( int nbuckets, unsigned int (*hash_function)(void*), int (*hash_key_compare)(void*, void*), size_t maxSize, long maxFiles)
{
    icl_hash_t *ht;
    int i;

    ht = (icl_hash_t*) malloc(sizeof(icl_hash_t));
    if(!ht) return NULL;

    ht->nentries = 0;
    ht->buckets = (icl_entry_t**)malloc(nbuckets * sizeof(icl_entry_t*));
    if(!ht->buckets) return NULL;

    ht->nbuckets = nbuckets;
    for(i=0;i<ht->nbuckets;i++)
        ht->buckets[i] = NULL;
    ht->hash_function = hash_function ? hash_function : hash_pjw;
    ht->hash_key_compare = hash_key_compare ? hash_key_compare : string_compare;
    ht->curr_size = 0;
    if (pthread_mutex_init(&ht->tableLock, NULL) != 0) {
        fprintf(stderr, "Errore nell'inizializzazione della lock\n");
        return NULL;
    }
    ht->max_size = maxSize;
    ht->max_files = maxFiles;
    return ht;
}

/**
 * Search for an entry in a hash table.
 *
 * @param ht -- the hash table to be searched
 * @param key -- the key of the item to search for
 *
 * @returns pointer to the data corresponding to the key.
 *   If the key was not found, returns NULL.
 */

void *
icl_hash_find(icl_hash_t *ht, void* key)
{
    icl_entry_t* curr;
    unsigned int hash_val;

    if(!ht || !key) return NULL;

    hash_val = (* ht->hash_function)(key) % ht->nbuckets;

    for (curr=ht->buckets[hash_val]; curr != NULL; curr=curr->next)
        if ( ht->hash_key_compare(curr->key, key))
            return(curr->data);

    return NULL;
}
int 
icl_hash_find_and_append(icl_hash_t *ht, void* key, void *data_to_append)
{
    icl_entry_t* curr;
    unsigned int hash_val;

    if(!ht || !key) return -1;
    if (!data_to_append) return -1;

    hash_val = (* ht->hash_function)(key) % ht->nbuckets;
    LOCK(&ht->tableLock);
    for (curr=ht->buckets[hash_val]; curr != NULL; curr=curr->next)
        if ( ht->hash_key_compare(curr->key, key)) {
            free(curr->data); //Libero la vecchia memoria
            curr->data = data_to_append; //Aggiorno il puntatore
        }
    UNLOCK(&ht->tableLock);
    return 0;
}

size_t get_current_size (icl_hash_t *ht) {
    return ht->curr_size;
}

/**
 * Append something to an item in the hash table.
 *
 * @param ht -- the hash table
 * @param key -- the key of the new item
 * @param data -- pointer to the new item's data
 *
 * @returns pointer to the new item.  Returns NULL on error.
 */


int append (icl_hash_t *ht, void *key, char *new_data, size_t size) { 
    if (!ht || !key) return -1;
    if (!new_data) return -1;
    char *data = (char*)icl_hash_find(ht, key);
    if (!data) {
        return -1;
    }

    /*Aggiorno la memoria lockando la hTable*/
    LOCK(&ht->tableLock);
    ht->curr_size = ht->curr_size + size; 
    UNLOCK(&ht->tableLock);

    size_t oldsize = strlen(data);
    char *res = malloc(oldsize+size+1);
    if (!res) {
        fprintf(stderr, "Errore nella malloc\n");
        return -1;
    }
    memcpy(res, data, oldsize);
    memcpy(res+oldsize, new_data, size+1);
    if (icl_hash_find_and_append(ht, key, (void*)res) == 0)
        return 0;
    return -1;
}


/**
 * Insert an item into the hash table.
 *
 * @param ht -- the hash table
 * @param key -- the key of the new item
 * @param data -- pointer to the new item's data
 *
 * @returns pointer to the new item.  Returns NULL on error.
 */



icl_entry_t *
icl_hash_insert(icl_hash_t *ht, void* key, void *data)
{
    icl_entry_t *curr;
    unsigned int hash_val;
    size_t len_of_data;
    if(!ht || !key) return NULL;
    if (data) {
        len_of_data = strnlen((char*)data, MAX_FILE_SIZE);
    }
    hash_val = (* ht->hash_function)(key) % ht->nbuckets;

    LOCK(&ht->tableLock);
    for (curr=ht->buckets[hash_val]; curr != NULL; curr=curr->next)
        if ( ht->hash_key_compare(curr->key, key))
            return(NULL); /* key already exists */
    
    /*Superata la memoria*/
    if (len_of_data > ht->max_size) {
        fprintf(stderr, "Errore - file troppo grande per la memoria dello storage\n"); //Se il file è più grande della capienza massima della tabella, non lo inserisco
        return NULL;
    }                                                                                   

    /* if key was not found */
    curr = (icl_entry_t*)malloc(sizeof(icl_entry_t));
    if(!curr) return NULL;

    curr->key = key;
    curr->data = data;
    curr->modified = 1;
    curr->next = ht->buckets[hash_val]; /* add at start */
    ht->buckets[hash_val] = curr;
    ht->nentries++;
    ht->curr_size += len_of_data;
    UNLOCK(&ht->tableLock);
    return curr;
}

/**
 * Free one hash table entry located by key (key and data are freed using functions).
 *
 * @param ht -- the hash table to be freed
 * @param key -- the key of the new item
 * @param free_key -- pointer to function that frees the key
 * @param free_data -- pointer to function that frees the data
 *
 * @returns 0 on success, -1 on failure.
 */
int icl_hash_delete(icl_hash_t *ht, void* key, void (*free_key)(void*), void (*free_data)(void*))
{
    icl_entry_t *curr, *prev;
    unsigned int hash_val;

    if(!ht || !key) return -1;
    hash_val = (* ht->hash_function)(key) % ht->nbuckets;

    prev = NULL;
    LOCK(&ht->tableLock);
    for (curr=ht->buckets[hash_val]; curr != NULL; )  {
        if ( ht->hash_key_compare(curr->key, key)) {
            if (prev == NULL) {
                ht->buckets[hash_val] = curr->next;
            } else {
                prev->next = curr->next;
            }
            if (*free_key && curr->key) (*free_key)(curr->key);
            if (*free_data && curr->data) {
                size_t old_size = strlen((char*)curr->data); //Peso in byte del file che sto per eliminare
                ht->curr_size = ht->curr_size - old_size; //Diminuisco l'attuale memoria utilizzata di 'old_size' byte
                (*free_data)(curr->data);
            } 
            ht->nentries--;
            free(curr);
            UNLOCK(&ht->tableLock);
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }
    UNLOCK(&ht->tableLock);
    return -1;
}

/**
 * Free hash table structures (key and data are freed using functions).
 *
 * @param ht -- the hash table to be freed
 * @param free_key -- pointer to function that frees the key
 * @param free_data -- pointer to function that frees the data
 *
 * @returns 0 on success, -1 on failure.
 */
int
icl_hash_destroy(icl_hash_t *ht, void (*free_key)(void*), void (*free_data)(void*))
{
    icl_entry_t *bucket, *curr, *next;
    int i;

    if(!ht) return -1;

    for (i=0; i<ht->nbuckets; i++) {
        bucket = ht->buckets[i];
        for (curr=bucket; curr!=NULL; ) {
            next=curr->next;
            if (*free_key && curr->key) (*free_key)(curr->key);
            if (*free_data && curr->data) {
                    (*free_data)(curr->data);
            }
            free(curr);
            curr=next;
        }
    }

    if(ht->buckets) free(ht->buckets);
    if(ht) free(ht);

    return 0;
}

/**
 * Dump the hash table's contents to the given socket descriptor.
 *
 * @param connFd -- the socket descriptor
 * @param ht -- the hash table to be dumped
 * @param n_of_files -- the number of files to read 
 *
 * @returns 0 on success, -1 on failure.
 */

int
icl_hash_dump(long connFd, icl_hash_t* ht, int n_of_files)
{
    icl_entry_t *bucket, *curr;
    int i;
    server_reply server_rep;
    memset(&server_rep, 0, sizeof(server_rep));
    if(!ht) return -1;
    int file_readed = 0;
    for(i=0; i<ht->nbuckets; i++) {
        bucket = ht->buckets[i];
        for(curr=bucket; curr!=NULL; ) {
            if(curr->key && file_readed <= n_of_files) {
                strcpy(server_rep.pathname, (char*)curr->key);
                strcpy(server_rep.data, (char*)curr->data);
                if (writen(connFd, &server_rep, sizeof(server_reply)) < 0) {
                    perror("writenDump");
                    return -1;
                }
                memset(&server_rep, 0, sizeof(server_reply));
                file_readed++;
            }
            curr=curr->next;
        }
    }

    return 0;
}

int
icl_hash_dump_2(FILE* stream, icl_hash_t* ht)
{
    icl_entry_t *bucket, *curr;
    int i;

    if(!ht) return -1;

    for(i=0; i<ht->nbuckets; i++) {
        bucket = ht->buckets[i];
        for(curr=bucket; curr!=NULL; ) {
            if(curr->key)
                fprintf(stream, "icl_hash_dump: %s: %s\n", (char *)curr->key, (char*)curr->data);
            curr=curr->next;
        }
    }

    return 0;
}


