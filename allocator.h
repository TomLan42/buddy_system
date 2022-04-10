#include "lru.h"
#define TOTAL_PAGES 512
#define MAX_LRU_ENTRIES 250


long long simu_physical_memory[TOTAL_PAGES];

typedef struct block_descriptor {
    int order;
    int first_page_address;
    struct block_descriptor *next;
}block_descriptor_t;

typedef struct free_list {
    int size;
    block_descriptor_t *head;
    block_descriptor_t *tail;
}free_list_t;


typedef struct buddy_allocator {
    lru_cache_t *active_list;
    lru_cache_t *inactive_list;
    free_list_t *free_list[10];
} buddy_allocator_t;

block_descriptor_t* seq_no_hash_table[1500];

// memory management methods
buddy_allocator_t* new_buddy_allocator(void);
void allocate_pages(buddy_allocator_t *allocator, int seq_no, int page_size);
void access_pages(buddy_allocator_t *allocator, int seq_no, int page_size);
void free_pages(buddy_allocator_t *allocator, int seq_no, int page_size);
block_descriptor_t *find_buddy_and_merge(buddy_allocator_t *allocator, int order, block_descriptor_t *free_block);


// free list manipulation methods
block_descriptor_t*  remove_head(free_list_t *list);
void remove_node(free_list_t *list, block_descriptor_t *node_to_remove);
void push_back(free_list_t *list, block_descriptor_t *block_descriptor);
void dump_free_list(free_list_t *list, int order);


// block_descriptor methods
block_descriptor_t *new_block_descriptor(int order, int address);