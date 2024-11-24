#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

// Block metadata structure
typedef struct block {
    size_t size;            // Size of the block
    struct block *next;     // Next block in list
    int free;              // Whether block is free
} block_t;

// Constants
#define BLOCK_SIZE sizeof(block_t)

// Global variables
static block_t *head = NULL;
static pthread_mutex_t malloc_lock = PTHREAD_MUTEX_INITIALIZER;

// Helper function to find a free block
static block_t *find_free_block(size_t size) {
    block_t *current = head;
    while (current != NULL) {
        if (current->free && current->size >= size) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Helper function to split a block
static void split_block(block_t *block, size_t size) {
    // Only split if the difference is big enough to hold a new block header
    // plus at least one byte of data
    if (block->size > size + BLOCK_SIZE + 1) {
        block_t *new_block = (block_t *)((char *)block + BLOCK_SIZE + size);
        new_block->size = block->size - size - BLOCK_SIZE;
        new_block->next = block->next;
        new_block->free = 1;

        block->size = size;
        block->next = new_block;
    }
}

// Helper function for coalescing
static void coalesce(void) {
    block_t *current = head;
    
    while (current != NULL && current->next != NULL) {
        if (current->free && current->next->free) {
            // Calculate if blocks are adjacent
            char *next_addr = (char *)current + BLOCK_SIZE + current->size;
            if (next_addr == (char *)current->next) {
                // Merge blocks
                current->size += BLOCK_SIZE + current->next->size;
                current->next = current->next->next;
                continue;  // Check if we can merge more
            }
        }
        current = current->next;
    }
}

void *mymalloc(size_t size) {
    if (size == 0) return NULL;

    pthread_mutex_lock(&malloc_lock);

    block_t *block = find_free_block(size);

    if (!block) {
        // No suitable block found, request new memory
        size_t request_size = BLOCK_SIZE + size;
        block_t *new_block = sbrk(request_size);
        
        if (new_block == (void *)-1) {
            pthread_mutex_unlock(&malloc_lock);
            return NULL;
        }

        new_block->size = size;
        new_block->next = NULL;
        new_block->free = 0;

        // Add to list
        if (!head) {
            head = new_block;
        } else {
            block_t *current = head;
            while (current->next != NULL) {
                current = current->next;
            }
            current->next = new_block;
        }
        block = new_block;
    } else {
        // Use existing block
        block->free = 0;
        split_block(block, size);
    }

    pthread_mutex_unlock(&malloc_lock);
    return (void *)(block + 1);
}

void *mycalloc(size_t nmemb, size_t size) {
    size_t total_size;
    
    // Check for multiplication overflow
    if (size != 0 && nmemb > SIZE_MAX / size) {
        return NULL;
    }
    
    total_size = nmemb * size;
    void *ptr = mymalloc(total_size);
    
    if (ptr != NULL) {
        memset(ptr, 0, total_size);
    }
    
    return ptr;
}

void myfree(void *ptr) {
    if (!ptr) return;

    pthread_mutex_lock(&malloc_lock);
    
    // Get the block metadata
    block_t *block = (block_t *)ptr - 1;
    block->free = 1;

    // Attempt to coalesce
    coalesce();
    
    pthread_mutex_unlock(&malloc_lock);
}