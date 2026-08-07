#ifndef BEAM_JIT_H
#define BEAM_JIT_H

#include "beam_core.h"
#include "beam_scheduler.h"

/**
 * @file beam_jit.h
 * @brief Public Opaque Interface for BEAM Bytecode JIT (Just-In-Time) Compiler Engine (C23 ISO Standard).
 */

typedef struct beam_jit_engine beam_jit_engine_t;
typedef beam_result_t (*beam_jit_fn_t)(beam_process_t* proc, Eterm* out_val);

BEAM_NODISCARD beam_jit_engine_t* beam_jit_engine_create(const beam_allocator_i* alloc);
void beam_jit_engine_destroy(beam_jit_engine_t* jit);

BEAM_NODISCARD beam_jit_fn_t beam_jit_compile_instructions(beam_jit_engine_t* jit, const beam_instruction_t* code, size_t code_len);
void beam_jit_free_fn(beam_jit_engine_t* jit, beam_jit_fn_t fn, size_t code_bytes);

#endif /* BEAM_JIT_H */
