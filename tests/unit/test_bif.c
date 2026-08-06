#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "beam_core.h"
#include "beam_scheduler.h"
#include "beam_bif_internal.h"
#include "mock_memory.h"

extern beam_result_t beam_bif_dispatch(size_t bif_index, beam_process_t* proc, const Eterm* args, int arity, Eterm* out_result);

void test_bif_dispatch(void) {
    printf("[UNIT TEST] Testing Built-In Functions (BIFs) Dispatch...\n");

    mock_memory_stats_t stats = {0};
    beam_allocator_i alloc = mock_memory_create(&stats);

    beam_process_t* proc = beam_process_create(555, 128, &alloc);
    assert(proc != NULL);

    /* Test BIF 0: erlang:self/0 */
    Eterm res_self = 0;
    beam_result_t res = beam_bif_dispatch(0, proc, NULL, 0, &res_self);
    assert(res == BEAM_OK);
    uint32_t pid = (uint32_t)(res_self >> 4);
    assert(pid == 555);

    /* Test BIF 1: erlang:+/2 */
    Eterm args_add[2] = { make_small_int(15), make_small_int(35) };
    Eterm res_add = 0;
    res = beam_bif_dispatch(1, proc, args_add, 2, &res_add);
    assert(res == BEAM_OK);
    (void)res;
    assert(eterm_to_small_int(res_add) == 50);

    beam_process_destroy(proc);

    assert(stats.alloc_count > 0);
    assert(stats.free_count == stats.alloc_count);
    printf("  [RESULT] BIF self/0 returned PID %u, BIF +/2 returned 50!\n", pid);
    printf("  [MEMORY] Clean memory stats! Allocs: %zu, Frees: %zu\n", stats.alloc_count, stats.free_count);
    printf("[PASSED] test_bif_dispatch\n");
}

int main(void) {
    printf("=========================================\n");
    printf(" RUNNING ISOLATED MODULE TEST: BIFs      \n");
    printf("=========================================\n");
    test_bif_dispatch();
    return 0;
}
