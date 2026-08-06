#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "beam_core.h"
#include "beam_memory.h"
#include "beam_scheduler.h"
#include "mock_memory.h"

void test_garbage_collection(void) {
    printf("[UNIT TEST] Testing Process Heap Garbage Collection...\n");

    mock_memory_stats_t stats = {0};
    beam_allocator_i alloc = mock_memory_create(&stats);

    beam_process_t* proc = beam_process_create(777, 64, &alloc);
    assert(proc != NULL);

    /* Allocate terms on process heap */
    Eterm* term1 = beam_process_alloc_heap(proc, 2);
    assert(term1 != NULL);
    term1[0] = make_small_int(100);
    term1[1] = make_small_int(200);

    /* Trigger GC on process heap */
    beam_result_t res = beam_gc_collect_process(proc);
    assert(res == BEAM_OK);
    (void)res;

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
