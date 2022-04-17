#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "allocator.h"

int main (void)

{

    // init file reading
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen("./test.dat", "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);



    // init allocator
    buddy_allocator_t *allocator = new_buddy_allocator();

    while ((read = getline(&line, &len, fp)) != -1) {
        printf("Processing request: %s \n", line);
        
        char** tokenized_string = str_split(line, '\t');

        char* request_type = tokenized_string[0];
        int request_seq_no = atoi(tokenized_string[1]);
        int request_page_size = atoi(tokenized_string[2]);
        
        switch (*request_type)
        {
            case 'A': 
                allocate_pages(allocator, request_seq_no, request_page_size);
                break;
            case 'X':
                access_pages(allocator, request_seq_no, request_page_size);
                break;
            case 'F':
                free_pages(allocator, request_seq_no, request_page_size);
                break;
        }
        // Remove first block to split it into halves
        for (int i = 0; i < 10; i++)
            dump_free_list(allocator->free_list[i],i);

        dump_lru_cache(allocator->inactive_list);
    }

    fclose(fp);
    if (line)
        free(line);
    exit(EXIT_SUCCESS);
   
    return 0;
}