#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "beam_core.h"
#include "beam_jit.h"
#include "mock_memory.h"

void test_jit_compilation(void) {
    printf("[UNIT TEST] Testing JIT Compiler Engine (x86_64 PROT_EXEC Machine Code Generation)...\n");

    mock_memory_stats_t stats = {0};
    beam_allocator_i alloc = mock_memory_create(&stats);

    beam_jit_engine_t* jit = beam_jit_engine_create(&alloc);
    assert(jit != NULL);

    uint8_t dummy_code[2] = {1, 0};
    beam_jit_fn_t compiled_fn = beam_jit_compile_instructions(jit, (const beam_instruction_t*)dummy_code, 2);
    assert(compiled_fn != NULL);

    /* Execute native x86_64 JIT machine code directly */
    Eterm out_val = 0;
    beam_result_t res = compiled_fn(NULL, &out_val);
    assert(res == BEAM_OK);
    (void)res;

    beam_jit_free_fn(jit, compiled_fn, 128);
    beam_jit_engine_destroy(jit);

    assert(stats.alloc_count > 0);
    assert(stats.free_count == stats.alloc_count);
    printf("  [RESULT] Executed native JIT compiled x86_64 machine code successfully!\n");
    printf("[PASSED] test_jit_compilation\n");
}

int main(void) {
    printf("=========================================\n");
    printf(" RUNNING ISOLATED MODULE TEST: JIT      \n");
    printf("=========================================\n");
    test_jit_compilation();
    return 0;
}
