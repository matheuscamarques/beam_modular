#ifndef MOCK_MEMORY_H
#define MOCK_MEMORY_H

#include "beam_memory.h"

typedef struct {
    size_t alloc_count;
    size_t free_count;
    size_t bytes_allocated;
} mock_memory_stats_t;

beam_allocator_i mock_memory_create(void* stats_ptr);

#endif /* MOCK_MEMORY_H */
