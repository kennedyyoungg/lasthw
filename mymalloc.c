#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stddef.h>
#include <debug.h>

// Block metadata structure
typedef struct block {
    size_t size;          
    struct block *next;   
} block_t;

// Global free list pointer
static block_t *free_list = NULL;

// Helper function to align memory sizes
static size_t align_size(size_t size) {
    return (size + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1);
}

// Wrapper for mmap to allocate memory
void *my_mmap(size_t increment) {
    debug_printf("Calling mmap with increment: %zu\n", increment);
    void *ptr = sbrk(increment); 
    if (ptr == (void *)-1) {
        debug_printf("mmap failed\n");
        return NULL;
    } else {
        debug_printf("mmap succeeded, returned pointer: %p\n", ptr);
        return ptr;
    }
}

// Add a block to the free list, maintaining order and coalescing
void add_to_free_list(block_t *block) {
    debug_printf("Adding block at: %p to free list\n", block);

    if (!free_list) {
        free_list = block;
        block->next = NULL;
        return;
    }

    block_t *prev = NULL;
    block_t *current = free_list;

    // Find the correct position to insert the block
    while (current && current < block) {
        prev = current;
        current = current->next;
    }

    // Insert the block into the list
    block->next = current;
    if (prev) {
        prev->next = block;
    } else {
        free_list = block;
    }

    // Coalesce with the next block if adjacent
    if (current && (void *)block + block->size == (void *)current) {
        debug_printf("Coalescing with next block at: %p\n", current);
        block->size += current->size;
        block->next = current->next;
    }

    // Coalesce with the previous block if adjacent
    if (prev && (void *)prev + prev->size == (void *)block) {
        debug_printf("Coalescing with previous block at: %p\n", prev);
        prev->size += block->size;
        prev->next = block->next;
    }
}

// Custom malloc implementation
void *mymalloc(size_t size) {
    debug_printf("Requested allocation size: %zu\n", size);

    if (size == 0) return NULL;

    // Align the requested size and include space for the block metadata
    size_t aligned_size = align_size(size);
    size_t total_size = aligned_size + sizeof(block_t);

    block_t *prev = NULL;
    block_t *current = free_list;

    while (current) {
        if (current->size >= total_size) {
            // Found a suitable block
            if (prev) {
                prev->next = current->next;
            } else {
                free_list = current->next;
            }
            debug_printf("Reusing block at: %p\n", current);
            return (void *)(current + 1);
        }
        prev = current;
        current = current->next;
    }

    // No suitable block found; allocate more memory
    block_t *new_block = my_mmap(total_size);
    if (!new_block) {
        debug_printf("Memory allocation failed!\n");
        return NULL;
    }

    new_block->size = total_size;
    new_block->next = NULL;

    debug_printf("Allocated block at: %p\n", new_block);
    return (void *)(new_block + 1);
}

// Custom free implementation and adds the block back to the free list with coalescing
void myfree(void *ptr) {
    if (!ptr) {
        debug_printf("Freeing NULL pointer\n");
        return;
    }

    block_t *block = (block_t *)ptr - 1; // Retrieve block metadata
    debug_printf("Freeing pointer at: %p\n", ptr);

    add_to_free_list(block); 
}

// Custom calloc implementation
void *mycalloc(size_t num, size_t size) {
    size_t total_size = num * size;
    void *ptr = mymalloc(total_size);
    if (ptr) {
        memset(ptr, 0, total_size); 
    }
    return ptr;
}