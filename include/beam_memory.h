#ifndef BEAM_MEMORY_H
#define BEAM_MEMORY_H

#include "beam_core.h"

/**
 * @file beam_memory.h
 * @brief Public Interface for Memory Allocation VTable and Generational GC (C23 ISO Standard).
 */

struct beam_allocator_i {
    void* ctx;
    void* (*alloc)(void* ctx, size_t size);
    void* (*realloc)(void* ctx, void* ptr, size_t size);
    void (*free)(void* ctx, void* ptr);
};

typedef struct {
    size_t total_allocated_bytes;
    size_t active_allocations;
    size_t peak_allocated_bytes;
} beam_memory_stats_t;

/* Memory Allocator Constructors & Destructors */
BEAM_NODISCARD beam_allocator_i beam_allocator_create_system(void);
BEAM_NODISCARD beam_allocator_i beam_allocator_create_arena(size_t capacity);
void beam_allocator_destroy(beam_allocator_i* alloc);
void beam_allocator_destroy_arena(beam_allocator_i* alloc);
beam_memory_stats_t beam_allocator_get_stats(const beam_allocator_i* alloc);

/* Generational Process Garbage Collector */
BEAM_NODISCARD beam_result_t beam_gc_collect_process(beam_process_t* proc);

#endif /* BEAM_MEMORY_H */
