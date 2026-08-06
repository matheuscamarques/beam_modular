#ifndef BEAM_ATOM_H
#define BEAM_ATOM_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "beam_allocator.h"

/**
 * @file beam_atom.h
 * @brief Public Opaque Interface for BEAM Atom Table.
 *
 * Internal structures are completely encapsulated in src/atom/beam_atom.c.
 * All memory allocations use the injected vtable interface.
 */

/* Opaque pointer definition */
typedef struct beam_atom_table beam_atom_table_t;

/* Status codes */
typedef enum {
    BEAM_ATOM_OK = 0,
    BEAM_ATOM_ERR_INVALID_ARG = -1,
    BEAM_ATOM_ERR_NO_MEMORY = -2,
    BEAM_ATOM_ERR_NOT_FOUND = -3,
    BEAM_ATOM_ERR_TABLE_FULL = -4
} beam_atom_result_t;

/**
 * Create a new isolated atom table instance.
 * @param alloc Memory allocator interface (vtable). Must not be NULL.
 * @param initial_capacity Initial hash table capacity (0 for default).
 * @return Opaque pointer to the atom table, or NULL on allocation failure.
 */
beam_atom_table_t* beam_atom_table_create(const beam_allocator_i* alloc, size_t initial_capacity);

/**
 * Destroy an atom table instance and free all allocated atom memory.
 * @param table Pointer to the atom table instance.
 */
void beam_atom_table_destroy(beam_atom_table_t* table);

/**
 * Insert or look up an atom in the table.
 * If the atom exists, returns its index. If not, it allocates and inserts it.
 * @param table Atom table instance.
 * @param name UTF-8 byte buffer of atom name.
 * @param len Length of atom name in bytes.
 * @param out_index Pointer to receive the atom index.
 * @return BEAM_ATOM_OK on success, or appropriate error code.
 */
beam_atom_result_t beam_atom_put(beam_atom_table_t* table, const char* name, size_t len, uint32_t* out_index);

/**
 * Find existing atom index by name without inserting.
 * @param table Atom table instance.
 * @param name UTF-8 byte buffer of atom name.
 * @param len Length of atom name in bytes.
 * @param out_index Pointer to receive the atom index.
 * @return BEAM_ATOM_OK if found, BEAM_ATOM_ERR_NOT_FOUND if absent.
 */
beam_atom_result_t beam_atom_find(const beam_atom_table_t* table, const char* name, size_t len, uint32_t* out_index);

/**
 * Get string representation of atom by index.
 * @param table Atom table instance.
 * @param index Atom index.
 * @param out_len Pointer to receive atom name length (optional, can be NULL).
 * @return Pointer to null-terminated atom string, or NULL if index is invalid.
 */
const char* beam_atom_get_name(const beam_atom_table_t* table, uint32_t index, size_t* out_len);

/**
 * Get the total number of atoms currently stored in the table.
 * @param table Atom table instance.
 * @return Number of atoms.
 */
size_t beam_atom_table_count(const beam_atom_table_t* table);

#endif /* BEAM_ATOM_H */
