#ifndef BEAM_CODE_SERVER_INTERNAL_H
#define BEAM_CODE_SERVER_INTERNAL_H

#include "beam_core.h"
#include "beam_memory.h"

#define MAX_LOADED_MODULES 128

typedef struct {
    char name[64];
    beam_file_t* file;
} module_entry_t;

struct beam_code_server {
    module_entry_t modules[MAX_LOADED_MODULES];
    size_t count;
    beam_allocator_i alloc;
};

#endif /* BEAM_CODE_SERVER_INTERNAL_H */
