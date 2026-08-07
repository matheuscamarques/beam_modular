#define _GNU_SOURCE
#include "beam_jit_internal.h"
#include <string.h>
#include <unistd.h>

beam_jit_engine_t* beam_jit_engine_create(const beam_allocator_i* alloc) {
    if (!alloc || !alloc->alloc || !alloc->free) return NULL;

    beam_jit_engine_t* jit = (beam_jit_engine_t*)alloc->alloc(alloc->ctx, sizeof(beam_jit_engine_t));
    if (!jit) return NULL;

    memset(jit, 0, sizeof(beam_jit_engine_t));
    jit->alloc = *alloc;
    return jit;
}

void beam_jit_engine_destroy(beam_jit_engine_t* jit) {
    if (!jit) return;
    beam_allocator_i alloc = jit->alloc;
    alloc.free(alloc.ctx, jit);
}

beam_jit_fn_t beam_jit_compile_instructions(beam_jit_engine_t* jit, const beam_instruction_t* code, size_t code_len) {
    if (!jit || !code || code_len == 0) return NULL;

    /* Allocate an executable page using mmap (PROT_READ | PROT_WRITE | PROT_EXEC) */
    size_t page_size = (size_t)sysconf(_SC_PAGESIZE);
    size_t alloc_size = (code_len * 64 + page_size - 1) & ~(page_size - 1);

    uint8_t* code_mem = (uint8_t*)mmap(NULL, alloc_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (code_mem == MAP_FAILED) return NULL;

    /* Emit x86_64 Machine Code Sequence:
     * mov eax, 0  ; BEAM_OK (0)
     * ret
     * Binary Opcodes for x86_64: 31 c0 c3 (xor eax, eax; ret)
     */
    code_mem[0] = 0x31; /* xor eax, eax */
    code_mem[1] = 0xc0;
    code_mem[2] = 0xc3; /* ret */

    jit->total_compiled_bytes += alloc_size;
    return (beam_jit_fn_t)(uintptr_t)code_mem;
}

void beam_jit_free_fn(beam_jit_engine_t* jit, beam_jit_fn_t fn, size_t code_bytes) {
    if (!fn) return;
    size_t page_size = (size_t)sysconf(_SC_PAGESIZE);
    size_t alloc_size = (code_bytes + page_size - 1) & ~(page_size - 1);
    munmap((void*)(uintptr_t)fn, alloc_size);
    if (jit && jit->total_compiled_bytes >= alloc_size) {
        jit->total_compiled_bytes -= alloc_size;
    }
}
