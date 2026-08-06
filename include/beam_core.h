#ifndef BEAM_CORE_H
#define BEAM_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @file beam_core.h
 * @brief Public Opaque Interface for Core Eterm Tagging, Loader, and Code Server.
 */

typedef uintptr_t Eterm;
typedef int32_t   beam_result_t;
typedef struct beam_allocator_i beam_allocator_i;

/* BEAM Status Codes */
#define BEAM_OK               0
#define BEAM_ERR_INVALID_ARG -1
#define BEAM_ERR_NO_MEMORY   -2
#define BEAM_ERR_NOT_FOUND   -3
#define BEAM_ERR_CORRUPT     -4
#define BEAM_ERR_HALT        -5
#define BEAM_ERR_BADARG      -6

/* Eterm Tagging Definitions */
#define TAG_PRIMARY_HEADER  0x0
#define TAG_PRIMARY_LIST    0x1
#define TAG_PRIMARY_BOXED   0x2
#define TAG_PRIMARY_IMMED1  0x3
#define PRIMARY_TAG_MASK    0x3

#define TAG_IMMED1_SMALL    0x3
#define TAG_IMMED1_ATOM     0x7
#define TAG_IMMED1_NIL      0x0B
#define TAG_IMMED1_PID      0x0F
#define IMMED1_TAG_MASK     0x0F

#define SUBTAG_TUPLE        0x00
#define HEADER_SUBTAG_MASK  0x3F

#define ETERM_NIL           ((Eterm)TAG_IMMED1_NIL)
#define ETERM_INVALID       ((Eterm)0)

/* Helper Functions */
static inline bool beam_is_small_int(Eterm term) {
    return (term & IMMED1_TAG_MASK) == TAG_IMMED1_SMALL;
}

static inline Eterm make_small_int(intptr_t val) {
    return (Eterm)((val << 4) | TAG_IMMED1_SMALL);
}

static inline intptr_t eterm_to_small_int(Eterm term) {
    return (intptr_t)term >> 4;
}

static inline bool beam_is_boxed(Eterm term) {
    return (term & PRIMARY_TAG_MASK) == TAG_PRIMARY_BOXED;
}

static inline bool beam_is_list(Eterm term) {
    return (term & PRIMARY_TAG_MASK) == TAG_PRIMARY_LIST;
}

/* Compound Terms & Opaque File / Code Server Types */
typedef struct beam_context beam_context_t;
typedef struct beam_instruction beam_instruction_t;
typedef struct beam_file beam_file_t;
typedef struct beam_code_server beam_code_server_t;
typedef struct beam_process beam_process_t;

/* Compound Term Allocation Headers */
Eterm beam_make_tuple(beam_process_t* proc, size_t arity, const Eterm* elements);
Eterm beam_make_list(beam_process_t* proc, Eterm head, Eterm tail);

bool beam_is_tuple(Eterm term);
size_t beam_tuple_arity(Eterm term);
Eterm beam_tuple_element(Eterm term, size_t index);

Eterm beam_list_head(Eterm term);
Eterm beam_list_tail(Eterm term);

/* BEAM File Container Interface */
beam_file_t* beam_file_parse(const uint8_t* buffer, size_t size, const beam_allocator_i* alloc);
void beam_file_destroy(beam_file_t* file);

const char* beam_file_get_module_name(const beam_file_t* file);
size_t beam_file_get_atom_count(const beam_file_t* file);
const char* beam_file_get_atom(const beam_file_t* file, size_t index);

/* Code Server & Module Registry Interface */
beam_code_server_t* beam_code_server_create(const beam_allocator_i* alloc);
void beam_code_server_destroy(beam_code_server_t* cs);

beam_result_t beam_code_server_register_module(beam_code_server_t* cs, const char* mod_name, beam_file_t* file);
beam_file_t* beam_code_server_lookup_module(const beam_code_server_t* cs, const char* mod_name);
size_t beam_code_server_module_count(const beam_code_server_t* cs);

#endif /* BEAM_CORE_H */
