#ifndef BEAM_GLOBAL_H
#define BEAM_GLOBAL_H

#include "beam_core.h"
#include "beam_memory.h"

/**
 * @file beam_global.h
 * @brief Public Opaque Interface for Global Shared State: Atom Table, ETS Tables, and Distributed Node Table (C23 ISO Standard).
 */

typedef struct beam_atom_table beam_atom_table_t;
typedef struct beam_ets_table beam_ets_table_t;
typedef struct beam_node_table beam_node_table_t;

/* Atom Table Operations */
BEAM_NODISCARD beam_atom_table_t* beam_atom_table_create(const beam_allocator_i* alloc, size_t initial_capacity);
void beam_atom_table_destroy(beam_atom_table_t* table);

BEAM_NODISCARD Eterm beam_atom_intern(beam_atom_table_t* table, const char* name);
BEAM_NODISCARD Eterm beam_atom_intern_length(beam_atom_table_t* table, const char* name, size_t len);
const char* beam_atom_lookup(const beam_atom_table_t* table, Eterm atom_term);
size_t beam_atom_table_count(const beam_atom_table_t* table);

/* Concurrent ETS Table Operations */
BEAM_NODISCARD beam_ets_table_t* beam_ets_table_create(const char* name, const beam_allocator_i* alloc);
void beam_ets_table_destroy(beam_ets_table_t* ets);

BEAM_NODISCARD beam_result_t beam_ets_insert(beam_ets_table_t* ets, Eterm key, Eterm value);
BEAM_NODISCARD beam_result_t beam_ets_lookup(const beam_ets_table_t* ets, Eterm key, Eterm* out_value);
BEAM_NODISCARD beam_result_t beam_ets_delete(beam_ets_table_t* ets, Eterm key);
size_t beam_ets_count(const beam_ets_table_t* ets);

/* Distributed Erlang Node Table Operations */
BEAM_NODISCARD beam_node_table_t* beam_node_table_create(const beam_allocator_i* alloc);
void beam_node_table_destroy(beam_node_table_t* nt);

BEAM_NODISCARD beam_result_t beam_node_table_connect(beam_node_table_t* nt, const char* node_name);
bool beam_node_table_is_connected(const beam_node_table_t* nt, const char* node_name);
size_t beam_node_table_count(const beam_node_table_t* nt);

#endif /* BEAM_GLOBAL_H */
