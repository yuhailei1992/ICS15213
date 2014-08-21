/* This header file contains the essential interfaces to proxy.c*/

#include "csapp.h"

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

typedef struct cache_manager {
    struct cache_block *head;
    unsigned cache_size;
    unsigned block_cnt;
    sem_t mutex;
} CM;

typedef struct cache_block {
    struct cache_block *next;
    struct cache_block *prev;
    char *id;
    unsigned size;
    char *data;
} CB;

CM *cache_create_new_cache ();

CB *cache_get (CM *Cache, char *uri);

int cache_check (CM *Cache, char *uri);

void cache_insert (CM *Cache, char *uri, char *data, unsigned size);