#ifndef BEAM_LOAD_INTERNAL_H
#define BEAM_LOAD_INTERNAL_H

#include "beam_core.h"
#include "beam_memory.h"
#include <stdint.h>
#include <stddef.h>

struct beam_file {
    char module_name[64];
    char** atom_table;
    size_t atom_count;
    beam_allocator_i alloc;
};

#endif /* BEAM_LOAD_INTERNAL_H */
