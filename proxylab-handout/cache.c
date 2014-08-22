/* cache
 * implemented a LRU cache with linked stach
 * used very very simple P and V operations to implement
 * read-write lock
 *
 * for more information, please refer to the header section in proxy.c
 */

/* Name = Hailei Yu
 * Andrew ID = haileiy
 */

#include "csapp.h"
#include "cache.h"

//=========================================functions
/* cache_create_new_cache :
 * creates a new cache manager and return it
 */
CM *cache_create_new_cache () {
    CM *Cache = Malloc (sizeof(CM));
    CB *addi_header = (CB *)malloc(sizeof(CB));
    Cache->head = addi_header;
    Cache->head->next = NULL;
    Cache->cache_size = 0;
    Cache->block_cnt = 0;
    sem_init(&Cache->mutex, 0, 1);
    return Cache;
}

/* cache_create_new_block
 * given the id, data and size, create a new block and returns it
 */
CB *cache_create_new_block(char *id, char *data, unsigned size) {
    printf("cache_create new block\n");
    CB *temp = (CB *)malloc(sizeof(CB));
    temp->id = (char *)malloc(strlen(id) + 1);
    strcpy(temp->id, id);
    temp->data = (char *)malloc(size);
    memcpy(temp->data, data, size);
    temp->size = size;
    temp->prev = NULL;
    temp->next = NULL;
    return temp;
}

/* cache_insert_after_head
 * given a cache block, insert it after the additional header
 * This functions is used to insert new blocks and update old blocks
 */
void cache_insert_after_head (CM *Cache, CB *blk) {
    printf("cache_insert after head\n");
    if (Cache->head->next == NULL) {
        Cache->head->next = blk;
        blk->prev = Cache->head;
        blk->next = NULL;
    }
    else {
        blk->next = Cache->head->next;
        Cache->head->next->prev = blk;
        Cache->head->next = blk;
        blk->prev = Cache->head;
    }
    Cache->cache_size += blk->size;
    Cache->block_cnt++;
    printf("insert_after_head...done!\n");
}

/* cache_detach_from_list
 * given a cache manager and a cache block
 * detach the block from the cache
 * but donnot destroy the block
 * This function is used with cacue_insert_after_head
 * to implement cache_move_to_head*/
void cache_detach_from_list (CM *Cache, CB *blk) {
    printf("cache_detach\n");
    if (blk->next == NULL) {
        CB *temp = blk->prev;
        temp->next = NULL;
    }
    else {
        CB *temp = blk->prev;
        temp->next = blk->next;
        blk->next->prev = temp;
    }
    Cache->cache_size -= blk->size;
    Cache->block_cnt--;
}
/* cache_move_to_head
 * moves a cache block to the head
 * meaning the cache block is recently used
 */
void cache_move_to_head (CM *Cache, CB *blk) {
    printf("cache_movetohead\n");
    P(&Cache->mutex);
    cache_detach_from_list(Cache, blk);
    cache_insert_after_head(Cache, blk);
    V(&Cache->mutex);
}

/* evict nodes at the end of the list
 * to control the cache's size within MAX_CACHE_SIZE
 */
void cache_evict (CM *Cache, int expected_size) {
    printf("cache_evict\n");
    P(&Cache->mutex);
    while (Cache->cache_size > expected_size) {
        CB *end = Cache->head;
        while (end->next != NULL) {
            end = end->next;
        }
        CB *end_prev = end->prev;
        end_prev->next = NULL;
        Cache->cache_size -= end->size;
        Cache->block_cnt --;
        Free(end);
    }
    V(&Cache->mutex);
    return;
}
/* cache_check:
 * checks if an uri has been cached
 * returns 1 is cached
 * returns 0 if not cached
 */
int cache_check (CM *Cache, char *uri) {
    printf("cache_check\n");
    CB *ptr = Cache->head->next;
    while (ptr) {
        if (!strcmp(uri, ptr->id)) {
            return 1;
        }
        ptr = ptr->next;
    }
    return 0;
}

/* cache_get
 * given an uri, fetch the block
 * this function is usually used with cache_check
 */
CB *cache_get (CM *Cache, char *uri) {
    printf("cache_get\n");
    CB *ptr = Cache->head->next;
    while (ptr) {
        if (!strcmp(uri, ptr->id)) {
            cache_move_to_head(Cache, ptr);
            return ptr;
        }
        ptr = ptr->next;
    }
    return NULL;
}
/* cache_insert:
 * given uri, data and size, create a new block and insert it
 * after the head
 */
void cache_insert (CM *Cache, char *uri, char *data, unsigned size) {
    printf("inserting cache\n");
    P(&Cache->mutex);
    if (size > MAX_OBJECT_SIZE) return;
    if (size + Cache->cache_size > MAX_CACHE_SIZE) {
        int expected_size = MAX_CACHE_SIZE - size;
        cache_evict(Cache, expected_size);
    }
    CB *new_block = cache_create_new_block(uri, data, size);
    cache_insert_after_head(Cache, new_block);
    V(&Cache->mutex);
}
