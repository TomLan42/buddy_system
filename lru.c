
#include <stdio.h>
#include <stdlib.h>
 
typedef struct lru_node {
    lru_node_t *prev;
    lru_node_t *next;
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
lru_node_t* new_lru_node(unsigned page_number)
{
    lru_node_t* temp = (lru_node_t*)malloc(sizeof(lru_node_t));
    temp->page_number = page_number;
 
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

    return lru_cache;
}
 
// create an empty hash_table of given capacity
hash_table_t* create_hash_table(int capacity)
{
    hash_table_t* hash_table = (hash_table_t*)malloc(sizeof(hash_table_t));
    hash_table->capacity = capacity;
 
    // array of pointers for referring lru nodes
     hash_table->array = (lru_node_t**)malloc(hash_table->capacity * sizeof(lru_node_t*));
 
    // initialize all hash entries as empty
    int i;
    for (i = 0; i < hash_table->capacity; ++i)
        hash_table->array[i] = NULL;
 
    return hash_table;
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
// return the evicted node
lru_node_t *evict(lru_cache_t* lru_cache)
{
    if (is_lru_empty(lru_cache))
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
 
// insert a node into lru cache
void insert(lru_cache_t* lru_cache, hash_table_t* hash_table, unsigned page_number)
{
    // if cache is full, remove the rear node
    if (is_lru_cache_full(lru_cache)) {
        // remove page from hash
        hash_table->array[lru_cache->rear->page_number] = NULL;
        evict(lru_cache);
    }
 
    // create node and insert into the front
    lru_node_t* new_node = create_node(page_number);
    new_node->next = lru_cache->front;
 
    // if lru cache is empty, change both front and rear pointers
    // else only change the front
    if (is_lru_cache_full(lru_cache)){
        lru_cache->rear = new_node;
        lru_cache->front = new_node;
    } else 
    {
        lru_cache->front->prev = new_node;
        lru_cache->front = new_node;
    }
 
    hash_table->array[page_number] = new_node;
 
    lru_cache->count++;
}
 


 // access a node with given page_number
 // 1) if exist, move the node to the front
 // 2) if not exist, insert a new node
void access(lru_cache_t* lru_cache, hash_table_t* hash_table, unsigned page_number)
{
    lru_node_t* req_node = hash_table->array[page_number];
 
    if (req_node == NULL)
        insert(lru_cache, hash_table, page_number);
 
    else if (req_node != lru_cache->front) {
        
        // unlink rquested node from its current location
        req_node->prev->next = req_node->next;
        if (req_node->next)
            req_node->next->prev = req_node->prev;
 
        // if requested node is rear, change rear
        if (req_node == lru_cache->rear) {
            lru_cache->rear = req_node->prev;
            lru_cache->rear->next = NULL;
        }
 
        // move req_node to front
        req_node->next = lru_cache->front;
        req_node->prev = NULL;
 
        // change previous front
        req_node->next->prev = req_node;
 
        // change front
        lru_cache->front = req_node;
    }
}
 
// // Driver program to test above functions
// int main()
// {
//     // Let cache can hold 4 pages
//     Queue* q = createQueue(4);
 
//     // Let 10 different pages can be requested (pages to be
//     // referenced are numbered from 0 to 9
//     Hash* hash = createHash(10);
 
//     // Let us refer pages 1, 2, 3, 1, 4, 5
//     ReferencePage(q, hash, 1);
//     ReferencePage(q, hash, 2);
//     ReferencePage(q, hash, 3);
//     ReferencePage(q, hash, 1);
//     ReferencePage(q, hash, 4);
//     ReferencePage(q, hash, 5);
 
//     // Let us print cache frames after the above referenced pages
//     printf("%d ", q->front->pageNumber);
//     printf("%d ", q->front->next->pageNumber);
//     printf("%d ", q->front->next->next->pageNumber);
//     printf("%d ", q->front->next->next->next->pageNumber);
 
//     return 0;
// }