#include "beam_load_internal.h"
#include <string.h>

static uint32_t read_u32_be(const uint8_t* ptr) {
    return ((uint32_t)ptr[0] << 24) |
           ((uint32_t)ptr[1] << 16) |
           ((uint32_t)ptr[2] << 8)  |
            (uint32_t)ptr[3];
}

beam_file_t* beam_file_parse(const uint8_t* buffer, size_t size, const beam_allocator_i* alloc) {
    if (!buffer || size < 12 || !alloc || !alloc->alloc || !alloc->free) {
        return NULL;
    }

    /* Check RIFF/IFF Magic Header: FOR1 ... BEAM */
    if (memcmp(&buffer[0], "FOR1", 4) != 0 || memcmp(&buffer[8], "BEAM", 4) != 0) {
        return NULL; /* Invalid BEAM file header */
    }

    beam_file_t* beam = (beam_file_t*)alloc->alloc(alloc->ctx, sizeof(beam_file_t));
    if (!beam) return NULL;

    memset(beam, 0, sizeof(beam_file_t));
    beam->alloc = *alloc;

    size_t pos = 12;
    while (pos + 8 <= size) {
        char chunk_id[5] = {0};
        memcpy(chunk_id, &buffer[pos], 4);
        uint32_t chunk_len = read_u32_be(&buffer[pos + 4]);
        pos += 8;

        if (pos + chunk_len > size) {
            break;
        }

        /* Process Atom Table Chunks: Atom or AtU8 */
        if (strcmp(chunk_id, "Atom") == 0 || strcmp(chunk_id, "AtU8") == 0) {
            uint32_t num_atoms = read_u32_be(&buffer[pos]);
            beam->atom_count = num_atoms;
            beam->atom_table = (char**)alloc->alloc(alloc->ctx, sizeof(char*) * num_atoms);
            if (beam->atom_table) {
                memset(beam->atom_table, 0, sizeof(char*) * num_atoms);
                size_t atom_pos = pos + 4;
                for (uint32_t i = 0; i < num_atoms; i++) {
                    uint8_t len = buffer[atom_pos];
                    atom_pos++;
                    char* name = (char*)alloc->alloc(alloc->ctx, len + 1);
                    if (name) {
                        memcpy(name, &buffer[atom_pos], len);
                        name[len] = '\0';
                        beam->atom_table[i] = name;
                        if (i == 0) {
                            strncpy(beam->module_name, name, sizeof(beam->module_name) - 1);
                        }
                    }
                    atom_pos += len;
                }
            }
        }

        /* 4-byte alignment padding for chunks */
        size_t pad = (4 - (chunk_len % 4)) % 4;
        pos += chunk_len + pad;
    }

    return beam;
}

const char* beam_file_get_module_name(const beam_file_t* beam) {
    return beam ? beam->module_name : NULL;
}

size_t beam_file_get_atom_count(const beam_file_t* beam) {
    return beam ? beam->atom_count : 0;
}

const char* beam_file_get_atom(const beam_file_t* beam, size_t index) {
    if (!beam || index >= beam->atom_count || !beam->atom_table) return NULL;
    return beam->atom_table[index];
}

void beam_file_destroy(beam_file_t* beam) {
    if (!beam) return;

    beam_allocator_i alloc = beam->alloc;
    if (beam->atom_table) {
        for (size_t i = 0; i < beam->atom_count; i++) {
            if (beam->atom_table[i]) {
                alloc.free(alloc.ctx, beam->atom_table[i]);
            }
        }
        alloc.free(alloc.ctx, beam->atom_table);
    }
    alloc.free(alloc.ctx, beam);
}

beam_module_t* beam_module_load_from_memory(const uint8_t* buffer, size_t size, const beam_allocator_i* alloc) {
    if (!buffer || !size || !alloc || !alloc->alloc || !alloc->free) {
        return NULL;
    }

    beam_module_t* mod = (beam_module_t*)alloc->alloc(alloc->ctx, sizeof(beam_module_t));
    if (!mod) return NULL;

    memset(mod, 0, sizeof(beam_module_t));
    mod->alloc = *alloc;

    mod->file = beam_file_parse(buffer, size, alloc);
    if (!mod->file) {
        alloc->free(alloc->ctx, mod);
        return NULL;
    }

    return mod;
}

void beam_module_destroy(beam_module_t* mod) {
    if (!mod) return;

    beam_allocator_i alloc = mod->alloc;
    if (mod->file) {
        beam_file_destroy(mod->file);
    }
    alloc.free(alloc.ctx, mod);
}
