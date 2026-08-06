#ifndef BEAM_GLOBAL_H
#define BEAM_GLOBAL_H

#include "beam_core.h"
#include "beam_memory.h"

/**
 * @file beam_global.h
 * @brief Public Opaque Interface for Global State (Atom Table & ETS Database).
 */

/* Opaque Atom Table Types */
typedef struct beam_atom_table beam_atom_table_t;

beam_atom_table_t* beam_atom_table_create(const beam_allocator_i* alloc, size_t initial_capacity);
void beam_atom_table_destroy(beam_atom_table_t* table);
beam_result_t beam_atom_put(beam_atom_table_t* table, const char* name, size_t len, uint32_t* out_index);
beam_result_t beam_atom_find(const beam_atom_table_t* table, const char* name, size_t len, uint32_t* out_index);
const char* beam_atom_get_name(const beam_atom_table_t* table, uint32_t index, size_t* out_len);
size_t beam_atom_table_count(const beam_atom_table_t* table);

/* Make Eterm atom from index */
static inline Eterm make_atom_eterm(uint32_t index) {
    return (Eterm)((index << 4) | TAG_IMMED1_ATOM);
}

/* Extract atom index from Eterm */
static inline uint32_t eterm_to_atom_index(Eterm term) {
    return (uint32_t)(term >> 4);
}

/* Opaque ETS Table Types */
typedef struct beam_ets_table beam_ets_table_t;

beam_ets_table_t* beam_ets_table_create(const char* name, const beam_allocator_i* alloc);
void beam_ets_table_destroy(beam_ets_table_t* table);
beam_result_t beam_ets_insert(beam_ets_table_t* table, Eterm key, Eterm value);
beam_result_t beam_ets_lookup(const beam_ets_table_t* table, Eterm key, Eterm* out_value);
beam_result_t beam_ets_delete(beam_ets_table_t* table, Eterm key);
size_t beam_ets_count(const beam_ets_table_t* table);

#endif /* BEAM_GLOBAL_H */
