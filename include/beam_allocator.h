#ifndef BEAM_ALLOCATOR_H
#define BEAM_ALLOCATOR_H

#include <stddef.h>

/**
 * @file beam_allocator.h
 * @brief Memory Allocator Interface (VTable) for Dependency Injection.
 *
 * Allows modules to allocate memory without depending directly on global
 * memory allocators (erts_alloc/malloc).
 */

typedef struct beam_allocator_i beam_allocator_i;

struct beam_allocator_i {
    void* (*alloc)(void* ctx, size_t size);
    void* (*realloc)(void* ctx, void* ptr, size_t new_size);
    void  (*free)(void* ctx, void* ptr);
    void* ctx;
};

#endif /* BEAM_ALLOCATOR_H */
