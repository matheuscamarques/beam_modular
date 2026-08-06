#include "beam_code_server_internal.h"
#include <string.h>

beam_code_server_t* beam_code_server_create(const beam_allocator_i* alloc) {
    if (!alloc || !alloc->alloc || !alloc->free) return NULL;

    beam_code_server_t* cs = (beam_code_server_t*)alloc->alloc(alloc->ctx, sizeof(beam_code_server_t));
    if (!cs) return NULL;

    memset(cs, 0, sizeof(beam_code_server_t));
    cs->alloc = *alloc;
    return cs;
}

void beam_code_server_destroy(beam_code_server_t* cs) {
    if (!cs) return;
    beam_allocator_i alloc = cs->alloc;

    for (size_t i = 0; i < cs->count; i++) {
        if (cs->modules[i].file) {
            beam_file_destroy(cs->modules[i].file);
        }
    }

    alloc.free(alloc.ctx, cs);
}

beam_result_t beam_code_server_register_module(beam_code_server_t* cs, const char* mod_name, beam_file_t* file) {
    if (!cs || !mod_name || !file) return BEAM_ERR_INVALID_ARG;
    if (cs->count >= MAX_LOADED_MODULES) return BEAM_ERR_NO_MEMORY;

    /* Check if already registered */
    for (size_t i = 0; i < cs->count; i++) {
        if (strcmp(cs->modules[i].name, mod_name) == 0) {
            return BEAM_OK;
        }
    }

    strncpy(cs->modules[cs->count].name, mod_name, sizeof(cs->modules[cs->count].name) - 1);
    cs->modules[cs->count].file = file;
    cs->count++;

    return BEAM_OK;
}

beam_file_t* beam_code_server_lookup_module(const beam_code_server_t* cs, const char* mod_name) {
    if (!cs || !mod_name) return NULL;

    for (size_t i = 0; i < cs->count; i++) {
        if (strcmp(cs->modules[i].name, mod_name) == 0) {
            return cs->modules[i].file;
        }
    }

    return NULL;
}

size_t beam_code_server_module_count(const beam_code_server_t* cs) {
    return cs ? cs->count : 0;
}
