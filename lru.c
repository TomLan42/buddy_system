
#include <stdio.h>
#include <stdlib.h>
#include "allocator.h"

// create a new lru_node
lru_node_t* new_lru_node(block_descriptor_t *block)
{
    lru_node_t* temp = (lru_node_t*)malloc(sizeof(lru_node_t));
    temp->block = block;
    temp->prev = NULL;
    temp->next = NULL;
 
    return temp;
}
 
// create an empty lru_cache of given capacity
lru_cache_t* new_lru_cache(int capacity)
{
    lru_cache_t* lru_cache = (lru_cache_t*)malloc(sizeof(lru_cache_t));
    lru_cache->count = 0;
    lru_cache->capacity = capacity;
    lru_cache->front = NULL;
    lru_cache->rear = NULL;

    // init hash
    lru_cache->hash_table = (lru_node_t**)malloc(1500 * sizeof(lru_node_t*));
 
    // initialize all hash entries as empty
    for (int i = 0; i < capacity; ++i)
       lru_cache->hash_table[i] = NULL;
    return lru_cache;
}
 
int is_lru_cache_full(lru_cache_t* lru_cache)
{
    return lru_cache->count == lru_cache->capacity;
}
 
int is_lru_cache_empty(lru_cache_t* lru_cache)
{
    return lru_cache->count == 0;
}
 
// evict a node from lru_cache
lru_node_t *lru_evict(lru_cache_t* lru_cache)
{
    if (is_lru_cache_empty(lru_cache))
        return NULL;
 
    // if it is the only node in list, change front
    if (lru_cache->front == lru_cache->rear)
        lru_cache->front = NULL;
 
    // change rear and remove the previous rear
    lru_node_t* evicted_node = lru_cache->rear;
    lru_cache->rear = lru_cache->rear->prev;
 
    if (lru_cache->rear)
        lru_cache->rear->next = NULL;
 
    lru_cache->count--;
    
    // dismantle evicted_node and return
    evicted_node->prev = NULL;
    return evicted_node;
}
 
// insert a node into lru cache, return the evicted node
lru_node_t *lru_insert(lru_cache_t* lru_cache, block_descriptor_t *block)
{
    lru_node_t *evected_node = NULL;

    // if cache is full, remove the rear node
    if (is_lru_cache_full(lru_cache)) {
        // remove page from hash
        lru_cache->hash_table[lru_cache->rear->block->seq_no] = NULL;
        evected_node = lru_evict(lru_cache);
    }
 
    // create node and insert into the front
    lru_node_t* new_node = new_lru_node(block);
    new_node->next = lru_cache->front;
 
    // if lru cache is empty, change both front and rear pointers
    // else only change the front
    if (is_lru_cache_empty(lru_cache)){
        lru_cache->rear = new_node;
        lru_cache->front = new_node;
    } else 
    {
        lru_cache->front->prev = new_node;
        lru_cache->front = new_node;
    }
 
    lru_cache->hash_table[block->seq_no] = new_node;
 
    lru_cache->count++;
    return evected_node;
}
 

 // remove a node from lru cache, return removed node
lru_node_t *lru_remove(lru_cache_t* lru_cache, int seq_no)
{
    if (is_lru_cache_empty(lru_cache))
        return NULL;
 
    lru_node_t* node_to_remove = lru_cache->hash_table[seq_no];
    if (node_to_remove == NULL)
        return NULL;
    

    // if node is the front
    if (lru_cache->front == node_to_remove) {
        lru_cache->front = node_to_remove->next;
    }

    // if node is the rear
    if (lru_cache->rear == node_to_remove) {
        lru_cache->rear = node_to_remove->prev;
    }

    lru_node_t* temp = node_to_remove->prev;
    if (node_to_remove->prev != NULL)
        node_to_remove->prev->next = node_to_remove->next;

    if (node_to_remove->next != NULL)
        node_to_remove->next->prev = temp;

    node_to_remove->prev = NULL;
    node_to_remove->next = NULL;

    lru_cache->hash_table[seq_no] = NULL;
    lru_cache->count--;

    return node_to_remove;
}

void dump_lru_cache(lru_cache_t *lru_cache) {

    lru_node_t *cur = lru_cache->front;
    
    printf("[LRU, count %d, cap %d]->", lru_cache->count, lru_cache->capacity);
    while (cur != NULL) {
        printf("[ seq_no: %d ]-> ", cur->block->seq_no);
        cur = cur->next;
    }

    printf("\n");
    return;
 }