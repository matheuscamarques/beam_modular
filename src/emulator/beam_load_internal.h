#ifndef BEAM_LOAD_INTERNAL_H
#define BEAM_LOAD_INTERNAL_H

#include "beam_core.h"
#include "beam_memory.h"
#include <stdint.h>
#include <stddef.h>

typedef struct beam_file beam_file_t;

struct beam_file {
    char module_name[64];
    char** atom_table;
    size_t atom_count;
    beam_allocator_i alloc;
};

beam_file_t* beam_file_parse(const uint8_t* buffer, size_t size, const beam_allocator_i* alloc);
const char* beam_file_get_module_name(const beam_file_t* beam);
size_t beam_file_get_atom_count(const beam_file_t* beam);
const char* beam_file_get_atom(const beam_file_t* beam, size_t index);
void beam_file_destroy(beam_file_t* beam);

#endif /* BEAM_LOAD_INTERNAL_H */
