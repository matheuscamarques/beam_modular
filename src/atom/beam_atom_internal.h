#ifndef BEAM_ATOM_INTERNAL_H
#define BEAM_ATOM_INTERNAL_H

#include "beam_atom.h"
#include <stdint.h>
#include <stddef.h>

#define BEAM_ATOM_DEFAULT_CAPACITY 256
#define BEAM_ATOM_MAX_LOAD_FACTOR 0.75f

typedef struct beam_atom_entry {
    char* name;
    size_t len;
    uint32_t hash;
    uint32_t index;
    struct beam_atom_entry* next;
} beam_atom_entry_t;

struct beam_atom_table {
    beam_allocator_i alloc;
    beam_atom_entry_t** buckets;
    beam_atom_entry_t** index_map; /* Array for O(1) lookup by index */
    size_t bucket_count;
    size_t count;
    size_t index_map_capacity;
};

#endif /* BEAM_ATOM_INTERNAL_H */
