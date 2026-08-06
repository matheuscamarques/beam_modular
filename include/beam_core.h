#ifndef BEAM_CORE_H
#define BEAM_CORE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * @file beam_core.h
 * @brief Public Core Definitions, Eterm representation, and Base Statuses.
 */

typedef struct beam_process beam_process_t;

/* Eterm primitive type: tagged word in Erlang VM */
typedef uintptr_t Eterm;

/* Tag masks and bits for Eterm */
#define PRIMARY_TAG_MASK     0x3
#define TAG_PRIMARY_HEADER   0x0
#define TAG_PRIMARY_LIST     0x1
#define TAG_PRIMARY_BOXED    0x2
#define TAG_PRIMARY_IMMED1   0x3

#define IMMED1_TAG_MASK      0xF
#define TAG_IMMED1_ATOM      0x3
#define TAG_IMMED1_SMALL     0xF
#define TAG_IMMED1_PID       0x7
#define TAG_IMMED1_NIL       0xB

#define HEADER_SUBTAG_MASK   0x3F
#define SUBTAG_TUPLE         0x00

/* Special Eterm values */
#define ETERM_NIL            ((Eterm)TAG_IMMED1_NIL)
#define ETERM_INVALID        ((Eterm)0)

/* Helper macros to create small integers */
static inline Eterm make_small_int(intptr_t val) {
    return (Eterm)(((uintptr_t)val << 4) | TAG_IMMED1_SMALL);
}

static inline intptr_t eterm_to_small_int(Eterm term) {
    return (intptr_t)term >> 4;
}

/* Tag check helpers */
static inline bool beam_is_small_int(Eterm term) {
    return (term & IMMED1_TAG_MASK) == TAG_IMMED1_SMALL;
}

static inline bool beam_is_nil(Eterm term) {
    return term == ETERM_NIL;
}

static inline bool beam_is_list(Eterm term) {
    return (term & PRIMARY_TAG_MASK) == TAG_PRIMARY_LIST;
}

static inline bool beam_is_boxed(Eterm term) {
    return (term & PRIMARY_TAG_MASK) == TAG_PRIMARY_BOXED;
}

/* Status codes */
typedef enum {
    BEAM_OK = 0,
    BEAM_ERR_INVALID_ARG = -1,
    BEAM_ERR_NO_MEMORY = -2,
    BEAM_ERR_NOT_FOUND = -3,
    BEAM_ERR_ALREADY_EXISTS = -4,
    BEAM_ERR_TIMEOUT = -5,
    BEAM_ERR_BADARG = -6,
    BEAM_ERR_HALT = -7
} beam_result_t;

/* Opaque Context Structure for global runtime instance */
typedef struct beam_context beam_context_t;

/* Instruction Opaque Types */
typedef struct beam_instruction beam_instruction_t;

/* Public Constructors for Compound Eterm Types */
Eterm beam_make_tuple(beam_process_t* proc, size_t arity, const Eterm* elements);
Eterm beam_make_list(beam_process_t* proc, Eterm head, Eterm tail);

/* Public Inspecting Helpers */
bool beam_is_tuple(Eterm term);
size_t beam_tuple_arity(Eterm term);
Eterm beam_tuple_element(Eterm term, size_t index);
Eterm beam_list_head(Eterm term);
Eterm beam_list_tail(Eterm term);

#endif /* BEAM_CORE_H */
