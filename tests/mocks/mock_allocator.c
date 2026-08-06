#include "mock_allocator.h"
#include <stdlib.h>

static void* mock_alloc(void* ctx, size_t size) {
    mock_allocator_stats_t* stats = (mock_allocator_stats_t*)ctx;
    if (stats) {
        stats->alloc_count++;
        stats->bytes_allocated += size;
    }
    return malloc(size);
}

static void* mock_realloc(void* ctx, void* ptr, size_t new_size) {
    mock_allocator_stats_t* stats = (mock_allocator_stats_t*)ctx;
    if (stats) {
        stats->alloc_count++;
        stats->bytes_allocated += new_size;
    }
    return realloc(ptr, new_size);
}

static void mock_free(void* ctx, void* ptr) {
    mock_allocator_stats_t* stats = (mock_allocator_stats_t*)ctx;
    if (stats && ptr) {
        stats->free_count++;
    }
    free(ptr);
}

beam_allocator_i mock_allocator_create(mock_allocator_stats_t* stats) {
    beam_allocator_i alloc;
    alloc.alloc = mock_alloc;
    alloc.realloc = mock_realloc;
    alloc.free = mock_free;
    alloc.ctx = stats;
    return alloc;
}
