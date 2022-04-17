#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "allocator.h"
#include "util.h"

buddy_allocator_t* new_buddy_allocator(void) {
    buddy_allocator_t* buddy_allocator = (buddy_allocator_t*)malloc(sizeof(buddy_allocator_t));

    buddy_allocator->active_list = new_lru_cache(MAX_LRU_ENTRIES);
    buddy_allocator->inactive_list = new_lru_cache(MAX_LRU_ENTRIES);
    

    for(int i = 0; i <= 9; i++) {
        buddy_allocator->free_list[i] = (free_list_t*)malloc(sizeof(free_list_t));
        buddy_allocator->free_list[i]->size = 0;
        buddy_allocator->free_list[i]->head = NULL;
    }

    // add the block of 512 to free list
    block_descriptor_t *init_block = new_block_descriptor(9, 0);
    buddy_allocator->free_list[9]->head = init_block;
    buddy_allocator->free_list[9]->size++;

    return buddy_allocator;
}

// allocate_pages allocates a block of contiguous pages for a process seq_no
// Three cases:
// 1) If there is a free block of the exact size, just allocate.
// 2) If there is not a free block of the exact size, but free block of bigger size,
//    split the bigger block and allocate.
// 3) If there is no suitable free block, try evict blocks from inactive list until
//    there is block matching the 2 cases above.
// After each allocation, the block will be moved into the inactive list.
void allocate_pages(buddy_allocator_t *allocator, int seq_no, int page_size) {
    // check if already allocated
    if (allocated_seq_no_hash_table[seq_no] != NULL || evicted_seq_no_hash_table[seq_no] != NULL) {
        printf("Sorry, already allocated \n");
        return;
    }

    block_descriptor_t *allocated_block = _allocate_block(allocator, get_order(page_size));

    while (allocated_block == NULL) {
        // if nothing to reclaim
        if (reclaim(allocator) != 0) {
            printf("Sorry, failed to allocate memory \n");
            return;
        }
        allocated_block = _allocate_block(allocator, get_order(page_size));
    }

    allocated_seq_no_hash_table[seq_no] = allocated_block;
    allocated_block->seq_no = seq_no;
    
    lru_node_t *evicted_node = lru_insert(allocator->inactive_list, allocated_block);
    if (evicted_node != NULL) {
        _free_block(allocator, evicted_node->block);
        allocated_seq_no_hash_table[evicted_node->block->seq_no] = NULL;
        evicted_seq_no_hash_table[evicted_node->block->seq_no] = evicted_node->block;
    }
}

block_descriptor_t *_allocate_block(buddy_allocator_t *allocator, int req_order) {
    // Case 1
    if (allocator->free_list[req_order]->size > 0)
	{
		// Remove block from free list
        block_descriptor_t* allocated_block = remove_head(allocator->free_list[req_order]); 
        printf("Memory from %d, order %d allocated \n", allocated_block->first_page_address,allocated_block->order);
               
        return allocated_block;
	}
	
    for(int i = req_order + 1; i < 10; i++)
    {
        // Case 2
        if(allocator->free_list[i]->size != 0) {
            block_descriptor_t* splitted_block = remove_head(allocator->free_list[i]);
            i--;
            // Iterative split
            for(; i >= req_order; i--)
            {
                // Divide block into two halves
                block_descriptor_t *block_1 = new_block_descriptor(i, splitted_block->first_page_address);
                block_descriptor_t *block_2 = new_block_descriptor(i, splitted_block->first_page_address+(0x1<<i));
                
                push_back(allocator->free_list[i], block_1);
                push_back(allocator->free_list[i], block_2);
                                
                // Remove first free block to further split
                splitted_block = remove_head(allocator->free_list[i]);
            }

        printf("Memory from %d, order %d allocated\n", splitted_block->first_page_address,splitted_block->order);
        
        return splitted_block;
        }
    }

    return NULL;
}


// access_pages access a page at a specific position from the block 
// allocated for seq_no
// Two cases:
// 1) If the allocated block is in physical memory, move the block to active list. If already
//    in the active list, do nothing.
// 2) If the allocated block is not in physical memory, bring the whole block back from the
//    evicted map. Move the block to active list.
void access_pages(buddy_allocator_t *allocator, int seq_no, int page_pos) {

    if(allocated_seq_no_hash_table[seq_no] == NULL && evicted_seq_no_hash_table[seq_no] == NULL) {
        printf("Not found, seq_no %d has been freed.\n", seq_no);
        return;
    }

    if (allocated_seq_no_hash_table[seq_no] != NULL) {
        lru_node_t  *promoted_node = lru_remove(allocator->inactive_list, seq_no);  
        if (promoted_node != NULL) {
            lru_insert(allocator->active_list, promoted_node->block);
        }
        return;;
    }


    // Page Fault!
    if (evicted_seq_no_hash_table[seq_no] != NULL) {
        int req_order = evicted_seq_no_hash_table[seq_no]->order;
        block_descriptor_t *swapped_in_block = _allocate_block(allocator, req_order);

        while (swapped_in_block == NULL) {
            // if nothing to reclaim
            if (reclaim(allocator) != 0) {
                printf("Sorry, failed to swap in memory \n");
                return;
            }
            swapped_in_block = _allocate_block(allocator, req_order);
        }

        allocated_seq_no_hash_table[seq_no] = swapped_in_block;
        swapped_in_block->seq_no = seq_no;
        evicted_seq_no_hash_table[seq_no] = NULL;

        if (allocator->active_list->hash_table[seq_no] == NULL) {
            lru_node_t *downgraded_node = lru_insert(allocator->active_list, swapped_in_block);
            lru_node_t *evicted_node = lru_insert(allocator->inactive_list, downgraded_node->block);
            if (evicted_node != NULL) {
                _free_block(allocator, evicted_node->block);
                allocated_seq_no_hash_table[evicted_node->block->seq_no] = NULL;
                evicted_seq_no_hash_table[evicted_node->block->seq_no] = evicted_node->block;
            }
        }
        
    }

    return;
}

// free_pages explicitly free a page at a specific position from the block 
// allocated for seq_no
// Three cases:
// 1) If all other pages of seq_no are freed:
//      a. if the block is still in physcial memory, release the block, remove
//         the block from any list and map.
//      b. if the block is not in physical memory, remove the block from any 
//         list and map.
//
// 2) If not all pages of seq_no are freed, record the free status for the seq_no
//    then do nothing.
void free_pages(buddy_allocator_t *allocator, int seq_no, int page_pos) {

    if(allocated_seq_no_hash_table[seq_no] == NULL && evicted_seq_no_hash_table[seq_no] == NULL){
        printf("Not found, seq_no %d has not been allocated.\n", seq_no);
        return;
    }

    if(allocated_seq_no_hash_table[seq_no] == NULL && evicted_seq_no_hash_table[seq_no] != NULL)
    {
       evicted_seq_no_hash_table[seq_no] = NULL;
       return;
    }

    _free_block(allocator, allocated_seq_no_hash_table[seq_no]);
    allocated_seq_no_hash_table[seq_no] = NULL;
    lru_remove(allocator->active_list, seq_no);
    lru_remove(allocator->inactive_list, seq_no);
    
    return;
}


void _free_block(buddy_allocator_t *allocator, block_descriptor_t *block_to_free) {
    // Size of block to be searched
    int order = block_to_free->order;
 
    // Add the block in free list
    push_back(allocator->free_list[order], block_to_free);
	printf("Memory from %d, order %d freed\n", block_to_free->first_page_address, block_to_free->order);

    block_descriptor_t *free_block = block_to_free;
   	for(int i = order; i < 9; i++)
	{
        block_descriptor_t *merged_block = _find_buddy_and_merge(allocator, i, free_block);
        if (merged_block == NULL) break;
        printf("Memory from %d, order %d freed -> merged to-> Memory from %d, order %d freed \n", free_block->first_page_address, free_block->order, merged_block->first_page_address, merged_block->order);
        free_block = merged_block;
	}

    return;
}

// find_buddy_and_merge do what the name sugguest :)
block_descriptor_t *_find_buddy_and_merge(buddy_allocator_t *allocator, int order, block_descriptor_t *free_block) {
    // Calculate buddy address, complement k-th bit
    int mask = 1 << free_block->order;
    int buddy_address = free_block->first_page_address ^ mask;
    int is_left_buddy = free_block->first_page_address & mask;
    
    // Search in free list to find it's buddy
    block_descriptor_t *cur = allocator->free_list[order]->head;

    while(cur != NULL)
    {
        // If buddy found and is also free, merge the buddies to make them one larger free memory block
        if (cur->first_page_address == buddy_address)
        {
            

            block_descriptor_t *merged_block;
            
            if (is_left_buddy)
                merged_block = new_block_descriptor(free_block->order+1, buddy_address);
            else
                merged_block = new_block_descriptor(free_block->order+1, free_block->first_page_address);
            // Add larger block to higher order free lsit
            
            push_back(allocator->free_list[order+1], merged_block);
            remove_node(allocator->free_list[order], cur);
            remove_node(allocator->free_list[order], free_block);

            return merged_block;
        }

        cur = cur->next;
    }

    return NULL;
}


int reclaim(buddy_allocator_t *allocator) {
    lru_node_t *evicted_node = lru_evict(allocator->inactive_list);
    if (evicted_node == NULL) // lru empty
        return -1;
    
    _free_block(allocator, evicted_node->block);
    return 0;
}



// free list (a single linked list) manipulation util
block_descriptor_t *remove_head(free_list_t *list) {
    if (list->size == 0) {
        return NULL;
    }
    block_descriptor_t *removed_node = list->head;
    list->head = list->head->next;
    removed_node->next = NULL;
    list->size--;
    return removed_node;
}

void remove_node(free_list_t *list, block_descriptor_t *node_to_remove) {
    if (list->size == 0) {
        return;
    }

    if (list->head == node_to_remove) {
        block_descriptor_t *temp = list->head;
        list->head = list->head->next;
        temp->next = NULL;
        list->size--;
        return;
    }

    block_descriptor_t *cur= list->head;
    while (cur->next != NULL) {
        if (cur->next == node_to_remove) {
            block_descriptor_t *temp = cur->next;
            cur->next = cur->next->next;
            temp = NULL;
            list->size--;
            return;
        }
        cur = cur->next;
    }
    
    return;
}

void push_back(free_list_t *list, block_descriptor_t *new_node) {
    if (list->head == NULL){
        list->head = new_node;
        list->size++;
        return;
    }
        
    block_descriptor_t *cur = list->head;
    while (cur->next != NULL) {
        cur = cur->next;
    }
    cur->next = new_node;
    list->size++;
    return;
}


void dump_free_list(free_list_t *list, int order) {
    block_descriptor_t *cur = list->head;
    
    printf("[ORDER %d, size %d]->", order, list->size);
    while (cur != NULL) {
        printf("[ address: %d, seq_no %d]-> ", cur->first_page_address, cur->seq_no);
        cur = cur->next;
    }

    printf("\n");
    return;
}

block_descriptor_t *new_block_descriptor(int order, int address) {
    block_descriptor_t *new_block = (block_descriptor_t*) malloc(sizeof(block_descriptor_t));
    new_block->order = order;
    new_block->first_page_address = address;
    new_block->next = NULL;
    new_block->seq_no = -1;

    return new_block;
}