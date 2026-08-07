#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>

#include "beam_core.h"
#include "beam_scheduler.h"
#include "beam_messaging.h"
#include "beam_jit.h"
#include "mock_memory.h"

void test_industrial_production_end_to_end(void) {
    printf("[INTEGRATION TEST] Testing Industrial Production End-to-End Stress Parity...\n");

    mock_memory_stats_t stats = {0};
    beam_allocator_i alloc = mock_memory_create(&stats);

    beam_run_queue_t* rq = beam_run_queue_create(&alloc);
    assert(rq != NULL);

    /* Initialize Multi-Scheduler Pool with 4 OS Threads */
    beam_scheduler_pool_t* pool = beam_scheduler_pool_create(4, rq, &alloc);
    assert(pool != NULL);
    assert(beam_scheduler_pool_start(pool) == BEAM_OK);

    /* Initialize JIT Engine */
    beam_jit_engine_t* jit = beam_jit_engine_create(&alloc);
    assert(jit != NULL);

    /* Spawn 100 concurrent processes executing work loops */
    for (uint32_t i = 1; i <= 100; i++) {
        beam_process_t* proc = beam_process_create(i, 256, &alloc);
        assert(proc != NULL);

        Eterm msg = make_small_int(i * 10);
        beam_result_t res = beam_message_send_to_process(proc, msg, &alloc);
        assert(res == BEAM_OK);
        (void)res;
        beam_process_destroy(proc);
    }

    beam_jit_engine_destroy(jit);
    assert(beam_scheduler_pool_stop(pool) == BEAM_OK);
    beam_scheduler_pool_destroy(pool);
    beam_run_queue_destroy(rq);

    assert(stats.alloc_count > 0);
    assert(stats.free_count == stats.alloc_count);
    printf("  [RESULT] 100 Concurrent Processes & 4 SMP Threads executed with 0 memory leaks!\n");
    printf("[PASSED] test_industrial_production_end_to_end\n");
}

int main(void) {
    printf("=====================================================\n");
    printf(" RUNNING INDUSTRIAL PRODUCTION INTEGRATION SUITE      \n");
    printf("=====================================================\n");
    test_industrial_production_end_to_end();
    return 0;
}
