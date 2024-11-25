#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))
#define MIN_BLOCK_SIZE 24  // Minimum size for a free block

typedef struct block_header {
    size_t size;            // Size of the block (including header)
    bool is_free;           // Whether the block is free
} block_header_t;

static void* heap_start = NULL;     // Start of our heap
static void* heap_end = NULL;       // End of our heap

// Helper function to get the header of a block from a pointer to the data
static block_header_t* get_block_header(void* ptr) {
    return (block_header_t*)((char*)ptr - sizeof(block_header_t));
}

// Helper function to get the data portion of a block from the header
static void* get_block_data(block_header_t* header) {
    return (void*)((char*)header + sizeof(block_header_t));
}

// Helper function to get the next header
static block_header_t* get_next_header(block_header_t* header) {
    if ((void*)((char*)header + header->size) >= heap_end) return NULL;
    return (block_header_t*)((char*)header + header->size);
}

// Initialize a block with given size
static void init_block(block_header_t* block, size_t size) {
    block->size = size;
    block->is_free = false;
}

// Helper to find a free block
static block_header_t* find_free_block(size_t size) {
    block_header_t* current = heap_start;
    
    while (current != NULL && (void*)current < heap_end) {
        if (current->is_free && current->size >= size) {
            return current;
        }
        current = get_next_header(current);
    }
    return NULL;
}

// Split a block if possible
static void split_block(block_header_t* block, size_t size) {
    size_t remaining = block->size - size;
    if (remaining >= MIN_BLOCK_SIZE) {
        // Create new block header after the allocation
        block_header_t* new_block = (block_header_t*)((char*)block + size);
        init_block(new_block, remaining);
        new_block->is_free = true;
        
        // Update original block
        block->size = size;
    }
}

void* mymalloc(size_t size) {
    if (size == 0) return NULL;
    
    // Calculate total size needed including header
    size_t total_size = ALIGN(sizeof(block_header_t) + size);
    if (total_size < MIN_BLOCK_SIZE) {
        total_size = MIN_BLOCK_SIZE;
    }
    
    block_header_t* block;
    
    // First allocation
    if (heap_start == NULL) {
        // Request initial chunk (4096 bytes or more if needed)
        size_t chunk_size = (total_size > 4096) ? total_size : 4096;
        heap_start = sbrk(chunk_size);
        if (heap_start == (void*)-1) return NULL;
        
        heap_end = (char*)heap_start + chunk_size;
        block = heap_start;
        init_block(block, chunk_size);
        block->is_free = true;
    }
    
    // Try to find a free block
    block = find_free_block(total_size);
    if (block) {
        block->is_free = false;
        split_block(block, total_size);
        return get_block_data(block);
    }
    
    // No suitable block found, request more memory
    block = sbrk(total_size);
    if (block == (void*)-1) return NULL;
    
    init_block(block, total_size);
    heap_end = (char*)block + total_size;
    
    return get_block_data(block);
}

void* mycalloc(size_t nmemb, size_t size) {
    size_t total_size = nmemb * size;
    
    // Check for overflow
    if (size != 0 && total_size / size != nmemb) return NULL;
    
    void* ptr = mymalloc(total_size);
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    return ptr;
}

void myfree(void* ptr) {
    if (!ptr) return;
    
    block_header_t* block = get_block_header(ptr);
    assert((void*)block >= heap_start && (void*)block < heap_end);
    
    block->is_free = true;
    
    // Coalesce with next block if it's free
    block_header_t* next = get_next_header(block);
    if (next && next->is_free) {
        block->size += next->size;
    }
}