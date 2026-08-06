#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "beam_core.h"
#include "beam_scheduler.h"
#include "mock_memory.h"

void test_priority_run_queue(void) {
    printf("[UNIT TEST] Testing Priority Run Queue & Preemption Order...\n");

    mock_memory_stats_t stats = {0};
    beam_allocator_i alloc = mock_memory_create(&stats);

    beam_run_queue_t* rq = beam_run_queue_create(&alloc);
    assert(rq != NULL);

    beam_process_t* proc_normal = beam_process_create(10, 64, &alloc);
    beam_process_t* proc_high   = beam_process_create(20, 64, &alloc);
    beam_process_t* proc_max    = beam_process_create(30, 64, &alloc);

    assert(proc_normal && proc_high && proc_max);

    /* Enqueue in reverse priority order */
    assert(beam_run_queue_enqueue(rq, proc_normal, BEAM_PRIO_NORMAL) == BEAM_OK);
    assert(beam_run_queue_enqueue(rq, proc_high,   BEAM_PRIO_HIGH)   == BEAM_OK);
    assert(beam_run_queue_enqueue(rq, proc_max,    BEAM_PRIO_MAX)    == BEAM_OK);

    assert(beam_run_queue_count(rq) == 3);

    /* Dequeue and verify priority scheduling (MAX -> HIGH -> NORMAL) */
    beam_process_t* p1 = beam_run_queue_dequeue(rq);
    assert(p1 != NULL && beam_process_get_pid(p1) == 30); /* MAX */
    (void)p1;

    beam_process_t* p2 = beam_run_queue_dequeue(rq);
    assert(p2 != NULL && beam_process_get_pid(p2) == 20); /* HIGH */
    (void)p2;

    beam_process_t* p3 = beam_run_queue_dequeue(rq);
    assert(p3 != NULL && beam_process_get_pid(p3) == 10); /* NORMAL */
    (void)p3;

    assert(beam_run_queue_count(rq) == 0);
    assert(beam_run_queue_dequeue(rq) == NULL);

    beam_process_destroy(proc_normal);
    beam_process_destroy(proc_high);
    beam_process_destroy(proc_max);
    beam_run_queue_destroy(rq);

    assert(stats.alloc_count > 0);
    assert(stats.free_count == stats.alloc_count);
    printf("  [MEMORY] Clean memory stats! Allocs: %zu, Frees: %zu\n", stats.alloc_count, stats.free_count);
    printf("[PASSED] test_priority_run_queue\n");
}

int main(void) {
    printf("=========================================\n");
    printf(" RUNNING ISOLATED MODULE TEST: RUN QUEUE \n");
    printf("=========================================\n");
    test_priority_run_queue();
    return 0;
}
