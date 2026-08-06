#ifndef BEAM_MEMORY_H
#define BEAM_MEMORY_H

#include "beam_core.h"

/**
 * @file beam_memory.h
 * @brief Memory Allocator (erts_alloc) and Garbage Collector Contracts.
 */

typedef struct beam_allocator_i beam_allocator_i;
typedef struct beam_process beam_process_t;

struct beam_allocator_i {
    void* (*alloc)(void* ctx, size_t size);
    void* (*realloc)(void* ctx, void* ptr, size_t new_size);
    void  (*free)(void* ctx, void* ptr);
    void* ctx;
};

/* Memory Statistics Structure */
typedef struct {
    size_t total_allocated_bytes;
    size_t total_freed_bytes;
    size_t active_allocations;
    size_t peak_allocated_bytes;
} beam_memory_stats_t;

/* Public Allocator Factory Functions */
beam_allocator_i beam_allocator_create_system(void);
beam_allocator_i beam_allocator_create_arena(size_t arena_size);
void beam_allocator_destroy(beam_allocator_i* alloc);
beam_memory_stats_t beam_allocator_get_stats(const beam_allocator_i* alloc);

/* Garbage Collector Public Interface */
beam_result_t beam_gc_collect_process(beam_process_t* proc);

#endif /* BEAM_MEMORY_H */
