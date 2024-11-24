#include <unistd.h>   
#include <stddef.h>   
#include <string.h>  
#include <stdio.h>    
#include <stdarg.h>   


typedef struct block {
    size_t size;            
    struct block *next;     
    int free;               
} block_t;

#define BLOCK_SIZE sizeof(block_t)

static block_t *head = NULL;  // Global pointer to the start of the linked list

// Function to print debug information
void debug_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

// Helper function to print the current state of the memory blocks
void print_memory_blocks() {
    block_t *current = head;
    printf("Memory Blocks:\n");
    while (current != NULL) {
        printf("Block at %p - Size: %zu, Free: %d\n", 
               (void*)current, current->size, current->free);
        current = current->next;
    }
}

// Malloc function
void *mymalloc(size_t size) {
    debug_printf("Malloc %zu bytes\n", size);
    block_t *current = head;
    block_t *last = NULL;

    // Traverse list to find a suitable free block
    while (current != NULL) {
        if (current->free && current->size >= size) {  
            current->free = 0;  
            return (void*)(current + 1);  
        }
        last = current;
        current = current->next;
    }

    // Requests more memory with sbrk
    block_t *new_block = sbrk(BLOCK_SIZE + size);
    if (new_block == (void*) -1) return NULL;  

    //Initialize new block
    new_block->size = size;
    new_block->next = NULL;
    new_block->free = 0;

    if (last) last->next = new_block;  
    else head = new_block;             

    return (void*)(new_block + 1);  
}

// Calloc function
void *mycalloc(size_t num, size_t size) {
    size_t total_size = num * size;
    void *ptr = mymalloc(total_size); 
    if (ptr) {
        memset(ptr, 0, total_size);  
    }
    debug_printf("Calloc %zu bytes\n", total_size);
    return ptr;
}

// Free function
void myfree(void *ptr) {
    if (!ptr) return;

    // Gets the block metadata by moving back by one `block_t`
    block_t *block = (block_t*)ptr - 1;
    block->free = 1;  
    debug_printf("Freed %zu bytes\n", block->size);
}