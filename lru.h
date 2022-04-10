
#include <stdio.h>
#include <stdlib.h>
 
typedef struct lru_node {
    struct lru_node *prev;
    struct lru_node *next;
    unsigned page_number;
} lru_node_t;
 
typedef struct lru_cache {
    unsigned count;
    unsigned capacity;
    lru_node_t *front;
    lru_node_t *rear;
} lru_cache_t;
 
// hash_table used for accessing lru_node with page number
typedef struct hash_table {
    int capacity; // how many pages can be there
    lru_node_t** array; // an array of lru nodes
} hash_table_t;
 
// create a new lru_node
lru_node_t* new_lru_node(unsigned page_number);
 
// create an empty lru_cache of given capacity
lru_cache_t* new_lru_cache(int capacity);
 
// create an empty hash_table of given capacity
hash_table_t* create_hash_table(int capacity);
 
int is_lru_cache_full(lru_cache_t* lru_cache);
 
int is_lru_cache_empty(lru_cache_t* lru_cache);
 
// evict a node from lru_cache
// return the evicted node
lru_node_t *evict(lru_cache_t* lru_cache);
 
// insert a node into lru cache
void insert(lru_cache_t* lru_cache, hash_table_t* hash_table, unsigned page_number);
 


 // access a node with given page_number
 // 1) if exist, move the node to the front
 // 2) if not exist, insert a new node
void access(lru_cache_t* lru_cache, hash_table_t* hash_table, unsigned page_number);