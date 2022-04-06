#include <stdlib.h>
#include "lru.c"


#define TOTAL_PAGES 512
#define MAX_LRU_ENTRIES 250


typedef int pages_size_t;

typedef unsigned long long page_address_t;


char simu_physical_memory[TOTAL_PAGES];

typedef struct buddy_allocator {
    lru_cache_t *active_list;
    lru_cache_t *inactive_list;
} buddy_allocator_t;


// memory management interface
void allocate_pages(buddy_allocator_t *allocator, pages_size_t page_size);
void access_page(buddy_allocator_t *allocator, int seq_no);
void free_pages(buddy_allocator_t *allocator, int seq_no);


// util
int page_address_2_seq_no(buddy_allocator_t *allocator, page_address_t page_address);
int seq_no_2_page_address(buddy_allocator_t *allocator, int seq_no);

