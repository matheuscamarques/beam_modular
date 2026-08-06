#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "beam_core.h"
#include "beam_scheduler.h"
#include "../../src/scheduler/erl_process_internal.h"
#include "mock_memory.h"

void test_process_lifecycle(void) {
    printf("[UNIT TEST] Testing Process Control Block (PCB) Lifecycle...\n");

    mock_memory_stats_t stats = {0};
    beam_allocator_i alloc = mock_memory_create(&stats);

    beam_process_t* proc = beam_process_create(101, 128, &alloc);
    assert(proc != NULL);
    assert(beam_process_get_pid(proc) == 101);
    assert(beam_process_get_state(proc) == BEAM_PROC_STATE_RUNNABLE);
    assert(beam_process_get_reductions(proc) == 4000);

    /* Allocate words on private heap */
    Eterm* words = beam_process_alloc_heap(proc, 10);
    assert(words != NULL);
    (void)words;
    assert(beam_process_heap_used(proc) == 10);

    /* Test reduction consumption */
    beam_process_consume_reductions(proc, 1500);
    assert(beam_process_get_reductions(proc) == 2500);

    /* Test unified stack push and pop */
    assert(beam_process_stack_push(proc, make_small_int(12345)) == BEAM_OK);
    Eterm popped = 0;
    assert(beam_process_stack_pop(proc, &popped) == BEAM_OK);
    assert(eterm_to_small_int(popped) == 12345);
    (void)popped;

    /* State transition */
    beam_process_set_state(proc, BEAM_PROC_STATE_RUNNING);
    assert(beam_process_get_state(proc) == BEAM_PROC_STATE_RUNNING);

    beam_process_destroy(proc);

    assert(stats.alloc_count > 0);
    assert(stats.free_count == stats.alloc_count);
    printf("  [MEMORY] Clean memory stats! Allocs: %zu, Frees: %zu\n", stats.alloc_count, stats.free_count);
    printf("[PASSED] test_process_lifecycle\n");
}

int main(void) {
    printf("=========================================\n");
    printf(" RUNNING ISOLATED MODULE TEST: SCHEDULER \n");
    printf("=========================================\n");
    test_process_lifecycle();
    return 0;
}
