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

/* Off-heap reference-counted binaries (ProcBin): payload lives outside the
 * process heap; the refcount is tied to process reachability. */
BEAM_NODISCARD Eterm beam_make_refc_binary(beam_process_t* proc, const void* data, size_t size);
BEAM_NODISCARD const void* beam_refc_binary_data(Eterm term, size_t* out_size);
BEAM_NODISCARD uint32_t beam_refc_binary_refcount(beam_process_t* proc, Eterm term);
void beam_refc_binary_retain(beam_process_t* proc, size_t size);

#endif /* BEAM_MEMORY_H */
