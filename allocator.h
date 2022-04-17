#define TOTAL_PAGES 512
#define MAX_LRU_ENTRIES 250

typedef struct block_descriptor {
    int order;
    int first_page_address;
    struct block_descriptor *next;
    int seq_no;
}block_descriptor_t;

typedef struct free_list {
    int size;
    block_descriptor_t *head;
    block_descriptor_t *tail;
}free_list_t;

typedef struct lru_node {
    struct lru_node *prev;
    struct lru_node *next;
    block_descriptor_t *block;
} lru_node_t;

typedef struct lru_cache {
    unsigned count; // count of pages
    unsigned capacity; // max capacity of pages
    lru_node_t** hash_table; // hash
    lru_node_t *front;
    lru_node_t *rear;
} lru_cache_t;

typedef struct buddy_allocator {
    lru_cache_t *active_list;
    lru_cache_t *inactive_list;
    free_list_t *free_list[10];
} buddy_allocator_t;

block_descriptor_t* allocated_seq_no_hash_table[1500];
block_descriptor_t* evicted_seq_no_hash_table[1500];


// memory management methods
buddy_allocator_t* new_buddy_allocator(void);
void allocate_pages(buddy_allocator_t *allocator, int seq_no, int page_size);
void access_pages(buddy_allocator_t *allocator, int seq_no);
void free_pages(buddy_allocator_t *allocator, int seq_no);
int reclaim(buddy_allocator_t *allocator);
block_descriptor_t *_find_buddy_and_merge(buddy_allocator_t *allocator, int order, block_descriptor_t *free_block);
block_descriptor_t *_allocate_block(buddy_allocator_t *allocator, int req_order);
void _free_block(buddy_allocator_t *allocator, block_descriptor_t *block_to_free);



// free list manipulation methods
block_descriptor_t*  remove_head(free_list_t *list);
void remove_node(free_list_t *list, block_descriptor_t *node_to_remove);
void push_back(free_list_t *list, block_descriptor_t *block_descriptor);
void dump_free_list(free_list_t *list, int order);


// block_descriptor methods
block_descriptor_t *new_block_descriptor(int order, int address);

// lru list methods
lru_node_t* new_lru_node(block_descriptor_t *block);
lru_cache_t* new_lru_cache(int capacity);
int is_lru_cache_full(lru_cache_t* lru_cache);
int is_lru_cache_empty(lru_cache_t* lru_cache);
lru_node_t *lru_insert(lru_cache_t* lru_cache, block_descriptor_t *block);
lru_node_t *lru_remove(lru_cache_t* lru_cache, int seq_no); 
lru_node_t *lru_evict(lru_cache_t* lru_cache);
void dump_lru_cache(lru_cache_t *lru_cache);