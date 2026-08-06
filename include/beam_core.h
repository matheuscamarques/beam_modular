#ifndef BEAM_CORE_H
#define BEAM_CORE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <assert.h>

/**
 * @file beam_core.h
 * @brief Public Opaque Interface for BEAM Modular Monolith Core Subsystem (C23 ISO Standard).
 */

/* Support [[nodiscard]] attribute in C23 standard */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
  #define BEAM_NODISCARD [[nodiscard]]
#elif defined(__GNUC__) || defined(__clang__)
  #define BEAM_NODISCARD __attribute__((warn_unused_result))
#else
  #define BEAM_NODISCARD
#endif

/* Core Types */
typedef uintptr_t Eterm;

/* Tag definitions */
#define TAG_PRIMARY_HEADER 0x00
#define TAG_PRIMARY_LIST   0x01
#define TAG_PRIMARY_BOXED  0x02
#define TAG_PRIMARY_IMMED1 0x03

#define TAG_IMMED1_SMALL_INT (0x00 << 2 | TAG_PRIMARY_IMMED1)
#define TAG_IMMED1_ATOM      (0x02 << 2 | TAG_PRIMARY_IMMED1)
#define TAG_IMMED1_PID       (0x03 << 2 | TAG_PRIMARY_IMMED1)
#define SUBTAG_TUPLE        0x00

#define ETERM_NIL (0x0F << 2 | TAG_PRIMARY_IMMED1)

/* Status Codes */
typedef enum {
    BEAM_OK = 0,
    BEAM_ERR_INVALID_ARG = -1,
    BEAM_ERR_NO_MEMORY = -2,
    BEAM_ERR_NOT_FOUND = -3,
    BEAM_ERR_HALT = -4,
    BEAM_ERR_CORRUPT_FILE = -5,
    BEAM_ERR_BADARG = -6
} beam_result_t;

/* C23 static_assert validation without 2nd argument message */
static_assert(sizeof(Eterm) == sizeof(uintptr_t));

/* Primitive Helpers */
static inline bool beam_is_small_int(Eterm term) {
    return (term & 0x0F) == TAG_IMMED1_SMALL_INT;
}

static inline Eterm make_small_int(intptr_t val) {
    return (Eterm)((val << 4) | TAG_IMMED1_SMALL_INT);
}

static inline intptr_t eterm_to_small_int(Eterm term) {
    return ((intptr_t)term) >> 4;
}

static inline bool beam_is_tuple(Eterm term) {
    if ((term & 0x03) != TAG_PRIMARY_BOXED) return false;
    Eterm* ptr = (Eterm*)(term & ~((uintptr_t)0x03));
    return ptr && ((*ptr & 0x3F) == SUBTAG_TUPLE);
}

static inline size_t beam_tuple_arity(Eterm term) {
    if (!beam_is_tuple(term)) return 0;
    Eterm* ptr = (Eterm*)(term & ~((uintptr_t)0x03));
    return (size_t)(*ptr >> 6);
}

static inline Eterm beam_tuple_element(Eterm term, size_t index) {
    if (!beam_is_tuple(term)) return 0;
    Eterm* ptr = (Eterm*)(term & ~((uintptr_t)0x03));
    return ptr[index + 1];
}

static inline bool beam_is_list(Eterm term) {
    return (term & 0x03) == TAG_PRIMARY_LIST;
}

static inline Eterm beam_list_head(Eterm term) {
    if (!beam_is_list(term)) return 0;
    Eterm* ptr = (Eterm*)(term & ~((uintptr_t)0x03));
    return ptr[0];
}

static inline Eterm beam_list_tail(Eterm term) {
    if (!beam_is_list(term)) return 0;
    Eterm* ptr = (Eterm*)(term & ~((uintptr_t)0x03));
    return ptr[1];
}

/* Opaque Context Forward Declarations */
typedef struct beam_context beam_context_t;
typedef struct beam_process beam_process_t;
typedef struct beam_allocator_i beam_allocator_i;
typedef struct beam_code_server beam_code_server_t;

/* Compound Term Allocation Prototypes */
Eterm beam_make_tuple(beam_process_t* proc, size_t arity, const Eterm* elements);
Eterm beam_make_list(beam_process_t* proc, Eterm head, Eterm tail);

/* BEAM Loader Interface */
typedef struct beam_module beam_module_t;
BEAM_NODISCARD beam_module_t* beam_module_load_from_memory(const uint8_t* buffer, size_t size, const beam_allocator_i* alloc);
void beam_module_destroy(beam_module_t* mod);

/* Orchestrator Entry Points */
BEAM_NODISCARD beam_context_t* beam_vm_create(const beam_allocator_i* alloc);
void beam_vm_destroy(beam_context_t* ctx);
beam_code_server_t* beam_vm_get_code_server(const beam_context_t* ctx);

/* Code Server Constructors */
BEAM_NODISCARD beam_code_server_t* beam_code_server_create(const beam_allocator_i* alloc);
void beam_code_server_destroy(beam_code_server_t* cs);
size_t beam_code_server_module_count(const beam_code_server_t* cs);

#endif /* BEAM_CORE_H */
