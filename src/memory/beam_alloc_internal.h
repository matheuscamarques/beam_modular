#ifndef BEAM_ALLOC_INTERNAL_H
#define BEAM_ALLOC_INTERNAL_H

#include "beam_memory.h"
#include <stdint.h>
#include <stddef.h>

typedef struct {
    size_t total_allocated;
    size_t total_freed;
    size_t active_allocs;
    size_t peak_bytes;
} system_alloc_state_t;

typedef struct {
    uint8_t* buffer;
    size_t capacity;
    size_t offset;
    size_t alloc_count;
} arena_alloc_state_t;

#endif /* BEAM_ALLOC_INTERNAL_H */
