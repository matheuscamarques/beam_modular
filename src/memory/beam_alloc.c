#include "beam_alloc_internal.h"
#include <stdlib.h>
#include <string.h>

/* Alignment constant for 64-bit architecture */
#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))

/* SYSTEM ALLOCATOR IMPLEMENTATION */
static void* sys_alloc(void* ctx, size_t size) {
    system_alloc_state_t* state = (system_alloc_state_t*)ctx;
    size = ALIGN(size);
    void* ptr = malloc(size);
    if (ptr && state) {
        state->total_allocated += size;
        state->active_allocs++;
        if (state->total_allocated - state->total_freed > state->peak_bytes) {
            state->peak_bytes = state->total_allocated - state->total_freed;
        }
    }
    return ptr;
}

static void* sys_realloc(void* ctx, void* ptr, size_t new_size) {
    system_alloc_state_t* state = (system_alloc_state_t*)ctx;
    new_size = ALIGN(new_size);
    void* new_ptr = realloc(ptr, new_size);
    if (new_ptr && state) {
        state->total_allocated += new_size;
    }
    return new_ptr;
}

static void sys_free(void* ctx, void* ptr) {
    system_alloc_state_t* state = (system_alloc_state_t*)ctx;
    if (ptr && state) {
        state->total_freed++;
        if (state->active_allocs > 0) {
            state->active_allocs--;
        }
    }
    free(ptr);
}

beam_allocator_i beam_allocator_create_system(void) {
    system_alloc_state_t* state = (system_alloc_state_t*)malloc(sizeof(system_alloc_state_t));
    if (state) {
        memset(state, 0, sizeof(system_alloc_state_t));
    }
    beam_allocator_i alloc;
    alloc.alloc = sys_alloc;
    alloc.realloc = sys_realloc;
    alloc.free = sys_free;
    alloc.ctx = state;
    return alloc;
}

/* ARENA ALLOCATOR IMPLEMENTATION */
static void* arena_alloc(void* ctx, size_t size) {
    arena_alloc_state_t* arena = (arena_alloc_state_t*)ctx;
    if (!arena || !arena->buffer) return NULL;

    size = ALIGN(size);
    if (arena->offset + size > arena->capacity) {
        return NULL; /* Out of arena space */
    }

    void* ptr = &arena->buffer[arena->offset];
    arena->offset += size;
    arena->alloc_count++;
    return ptr;
}

static void* arena_realloc(void* ctx, void* ptr, size_t new_size) {
    (void)ptr;
    /* Realloc in simple bump-pointer arena delegates to new alloc */
    return arena_alloc(ctx, new_size);
}

static void arena_free(void* ctx, void* ptr) {
    (void)ctx;
    (void)ptr;
    /* Arena memory freed all at once during destroy */
}

beam_allocator_i beam_allocator_create_arena(size_t arena_size) {
    arena_alloc_state_t* arena = (arena_alloc_state_t*)malloc(sizeof(arena_alloc_state_t));
    if (arena) {
        arena->buffer = (uint8_t*)malloc(arena_size);
        arena->capacity = arena_size;
        arena->offset = 0;
        arena->alloc_count = 0;
    }

    beam_allocator_i alloc;
    alloc.alloc = arena_alloc;
    alloc.realloc = arena_realloc;
    alloc.free = arena_free;
    alloc.ctx = arena;
    return alloc;
}

void beam_allocator_destroy(beam_allocator_i* alloc) {
    if (!alloc || !alloc->ctx) return;

    if (alloc->alloc == sys_alloc) {
        free(alloc->ctx);
    } else if (alloc->alloc == arena_alloc) {
        arena_alloc_state_t* arena = (arena_alloc_state_t*)alloc->ctx;
        if (arena->buffer) free(arena->buffer);
        free(arena);
    }
    alloc->ctx = NULL;
}

beam_memory_stats_t beam_allocator_get_stats(const beam_allocator_i* alloc) {
    beam_memory_stats_t stats = {0};
    if (!alloc || !alloc->ctx) return stats;

    if (alloc->alloc == sys_alloc) {
        system_alloc_state_t* state = (system_alloc_state_t*)alloc->ctx;
        stats.total_allocated_bytes = state->total_allocated;
        stats.active_allocations = state->active_allocs;
        stats.peak_allocated_bytes = state->peak_bytes;
    } else if (alloc->alloc == arena_alloc) {
        arena_alloc_state_t* arena = (arena_alloc_state_t*)alloc->ctx;
        stats.total_allocated_bytes = arena->offset;
        stats.active_allocations = arena->alloc_count;
        stats.peak_allocated_bytes = arena->capacity;
    }

    return stats;
}
