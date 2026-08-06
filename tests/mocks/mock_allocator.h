#ifndef MOCK_ALLOCATOR_H
#define MOCK_ALLOCATOR_H

#include "beam_allocator.h"
#include <stddef.h>

typedef struct {
    size_t alloc_count;
    size_t free_count;
    size_t bytes_allocated;
    size_t bytes_freed;
} mock_allocator_stats_t;

/**
 * Creates a mock memory allocator interface that wraps standard malloc/free
 * while tracking allocation counts and detecting memory leaks.
 */
beam_allocator_i mock_allocator_create(mock_allocator_stats_t* stats);

#endif /* MOCK_ALLOCATOR_H */
