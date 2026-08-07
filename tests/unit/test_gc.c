#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "beam_core.h"
#include "beam_memory.h"
#include "beam_scheduler.h"
#include "../../src/scheduler/erl_process_internal.h"
#include "mock_memory.h"

void test_garbage_collection(void) {
    printf("[UNIT TEST] Testing Process Heap Garbage Collection...\n");

    mock_memory_stats_t stats = {0};
    beam_allocator_i alloc = mock_memory_create(&stats);

    beam_process_t* proc = beam_process_create(777, 64, &alloc);
    assert(proc != NULL);

    /* Allocate terms on process heap */
    Eterm* tuple_buf = beam_process_alloc_heap(proc, 3);
    assert(tuple_buf != NULL);
    tuple_buf[0] = make_small_int(2); /* arity = 2 */
    tuple_buf[1] = make_small_int(100);
    tuple_buf[2] = make_small_int(200);

    Eterm tuple_term = (Eterm)(((uintptr_t)tuple_buf) | TAG_PRIMARY_BOXED);

    /* Allocate dead garbage term on heap that is NOT pointed to by any root register */
    Eterm* dead_buf = beam_process_alloc_heap(proc, 2);
    assert(dead_buf != NULL);
    dead_buf[0] = make_small_int(9999);
    dead_buf[1] = make_small_int(8888);

    /* Store tuple_term in X[0] register root */
    proc->frame.x_regs[0] = tuple_term;

    size_t heap_top_before = proc->heap_top;
    assert(heap_top_before == 5);
    (void)heap_top_before;

    /* Trigger Cheney GC on process heap */
    beam_result_t res = beam_gc_collect_process(proc);
    assert(res == BEAM_OK);
    (void)res;

    /* Verify GC compacted live roots (tuple_term moved) and dropped dead_buf */
    assert(proc->heap_top == 3); /* Only tuple (3 words) preserved, dead_buf reclaimed */
    Eterm gc_tuple = proc->frame.x_regs[0];
    assert(beam_is_tuple(gc_tuple));
    assert(beam_tuple_arity(gc_tuple) == 2);
    assert(eterm_to_small_int(beam_tuple_element(gc_tuple, 0)) == 100);
    assert(eterm_to_small_int(beam_tuple_element(gc_tuple, 1)) == 200);
    (void)gc_tuple;

    beam_process_destroy(proc);

    assert(stats.alloc_count > 0);
    assert(stats.free_count == stats.alloc_count);
    printf("  [MEMORY] Clean memory stats! Allocs: %zu, Frees: %zu\n", stats.alloc_count, stats.free_count);
    printf("[PASSED] test_garbage_collection\n");
}

int main(void) {
    printf("=========================================\n");
    printf(" RUNNING ISOLATED MODULE TEST: GC        \n");
    printf("=========================================\n");
    test_garbage_collection();
    return 0;
}
