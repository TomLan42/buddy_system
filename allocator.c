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

	int req_order = get_order(page_size);
	
	// Case 1
	if (allocator->free_list[req_order]->size > 0)
	{
		// Remove block from free list
        block_descriptor_t* allocated_block = remove_head(allocator->free_list[req_order]); 
        printf("Memory from %d, order %d allocated \n", allocated_block->first_page_address,allocated_block->order);
               
        seq_no_hash_table[seq_no] = allocated_block;
        allocated_block->seq_no = seq_no;
        lru_insert(allocator->inactive_list, allocated_block);
        return;
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
        
        seq_no_hash_table[seq_no] = splitted_block;
        splitted_block->seq_no = seq_no;
        lru_insert(allocator->inactive_list, splitted_block);
        return;
        }
    }

    // Case 3
    printf("Sorry, failed to allocate memory \n");
		
}


// access_pages access a page at a specific position from the block 
// allocated for seq_no
// Four cases:
// 1) If the allocated block is in physical memory, move the block to active list. If already
//    in the active list, do nothing.
// 2) If the allocated block is not in physical memory, bring the whole block back from the
//    evicted map. Move the block to active list.
void access_pages(buddy_allocator_t *allocator, int seq_no, int page_pos) {

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
    // If no such starting address available
    if(seq_no_hash_table[seq_no] == NULL)
    {
       printf("Not found, seq_no %d has not been allocated \n", seq_no);
       return;
    }
     
    // Size of block to be searched
    int order = seq_no_hash_table[seq_no]->order;
 
    // Add the block in free list
    push_back(allocator->free_list[order], seq_no_hash_table[seq_no]);
    dump_free_list(allocator->free_list[order],order);

	printf("Memory from %d, order %d freed\n", seq_no_hash_table[seq_no]->first_page_address, seq_no_hash_table[seq_no]->order);

    block_descriptor_t *free_block = seq_no_hash_table[seq_no];
   	for(int i = order; i < 9; i++)
	{
        block_descriptor_t *merged_block = find_buddy_and_merge(allocator, i, free_block);
        if (merged_block == NULL) break;
        printf("Memory from %d, order %d freed ->merged to-> Memory from %d, order %d freed \n", seq_no_hash_table[seq_no]->first_page_address, seq_no_hash_table[seq_no]->order, merged_block->first_page_address, merged_block->order);
        free_block = merged_block;
	}

    // Remove the key existence from map
    lru_remove(allocator->inactive_list, seq_no);
    seq_no_hash_table[seq_no]->seq_no = -1;
    seq_no_hash_table[seq_no] = NULL;
}


// find_buddy_and_merge do what the name sugguest :)
block_descriptor_t *find_buddy_and_merge(buddy_allocator_t *allocator, int order, block_descriptor_t *free_block) {
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