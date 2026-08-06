#ifndef BEAM_CODE_SERVER_INTERNAL_H
#define BEAM_CODE_SERVER_INTERNAL_H

#include "beam_core.h"
#include "beam_memory.h"
#include "beam_load_internal.h"

#define MAX_LOADED_MODULES 128

typedef struct beam_code_server beam_code_server_t;

typedef struct {
    char name[64];
    beam_file_t* file;
} module_entry_t;

struct beam_code_server {
    module_entry_t modules[MAX_LOADED_MODULES];
    size_t count;
    beam_allocator_i alloc;
};

beam_code_server_t* beam_code_server_create(const beam_allocator_i* alloc);
void beam_code_server_destroy(beam_code_server_t* cs);

beam_result_t beam_code_server_register_module(beam_code_server_t* cs, const char* mod_name, beam_file_t* file);
beam_file_t* beam_code_server_lookup_module(const beam_code_server_t* cs, const char* mod_name);
size_t beam_code_server_module_count(const beam_code_server_t* cs);

#endif /* BEAM_CODE_SERVER_INTERNAL_H */
