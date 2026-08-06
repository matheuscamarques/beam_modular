#include "beam_memory.h"
#include <stdlib.h>

typedef struct {
    size_t alloc_count;
    size_t free_count;
    size_t bytes_allocated;
} mock_memory_stats_t;

static void* mock_alloc_fn(void* ctx, size_t size) {
    mock_memory_stats_t* stats = (mock_memory_stats_t*)ctx;
    if (stats) {
        stats->alloc_count++;
        stats->bytes_allocated += size;
    }
    return malloc(size);
}

static void* mock_realloc_fn(void* ctx, void* ptr, size_t new_size) {
    mock_memory_stats_t* stats = (mock_memory_stats_t*)ctx;
    if (stats) {
        stats->alloc_count++;
        stats->bytes_allocated += new_size;
    }
    return realloc(ptr, new_size);
}

static void mock_free_fn(void* ctx, void* ptr) {
    mock_memory_stats_t* stats = (mock_memory_stats_t*)ctx;
    if (stats && ptr) {
        stats->free_count++;
    }
    free(ptr);
}

beam_allocator_i mock_memory_create(void* stats_ptr) {
    beam_allocator_i alloc;
    alloc.alloc = mock_alloc_fn;
    alloc.realloc = mock_realloc_fn;
    alloc.free = mock_free_fn;
    alloc.ctx = stats_ptr;
    return alloc;
}
