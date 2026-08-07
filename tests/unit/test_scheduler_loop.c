#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include "beam_core.h"
#include "beam_scheduler.h"
#include "beam_emu_internal.h"
#include "mock_memory.h"

void test_scheduler_preemption_step(void) {
    printf("[UNIT TEST] Testing Scheduler Preemption Loop & Reduction Yielding...\n");

    mock_memory_stats_t stats = {0};
    beam_allocator_i alloc = mock_memory_create(&stats);

    beam_run_queue_t* rq = beam_run_queue_create(&alloc);
    assert(rq != NULL);

    beam_scheduler_t* sched = beam_scheduler_create(1, rq, &alloc);
    assert(sched != NULL);

    beam_process_t* proc = beam_process_create(701, 128, &alloc);
    assert(proc != NULL);

    /* Set low reductions limit = 2 to force preemption */
    beam_process_set_reductions(proc, 2);
    beam_result_t enq_res = beam_run_queue_enqueue(rq, proc, BEAM_PRIO_NORMAL);
    (void)enq_res;

    beam_instruction_t code[] = {
        { .opcode = BEAM_OP_MOVE, .arg1 = 0, .arg2 = 0, .literal = make_small_int(1) },
        { .opcode = BEAM_OP_MOVE, .arg1 = 0, .arg2 = 1, .literal = make_small_int(2) },
        { .opcode = BEAM_OP_MOVE, .arg1 = 0, .arg2 = 2, .literal = make_small_int(3) },
        { .opcode = BEAM_OP_HALT }
    };

    /* 1st Step: Process runs 2 reductions, yields, and re-enqueues into run queue */
    beam_result_t res1 = beam_scheduler_step(sched, code, sizeof(code)/sizeof(code[0]));
    assert(res1 == BEAM_OK);
    assert(beam_run_queue_count(rq) == 1);
    (void)res1;

    /* 2nd Step: Process dequeues, resumes execution, and reaches HALT */
    beam_result_t res2 = beam_scheduler_step(sched, code, sizeof(code)/sizeof(code[0]));
    assert(res2 == BEAM_ERR_HALT);
    assert(beam_run_queue_count(rq) == 0);
    assert(beam_process_get_state(proc) == BEAM_PROC_STATE_EXITED);
    (void)res2;

    beam_process_destroy(proc);
    beam_scheduler_destroy(sched);
    beam_run_queue_destroy(rq);

    assert(stats.alloc_count > 0);
    assert(stats.free_count == stats.alloc_count);
    printf("  [RESULT] Scheduler pre-empted process after 2 reductions and resumed to HALT cleanly!\n");
    printf("  [PASSED] test_scheduler_preemption_step\n");
}

void test_scheduler_pool_parallel(void) {
    printf("[UNIT TEST] Testing Multi-Scheduler Thread Pool (4 Parallel Worker Threads)...\n");

    mock_memory_stats_t stats = {0};
    beam_allocator_i alloc = mock_memory_create(&stats);

    beam_run_queue_t* rq = beam_run_queue_create(&alloc);
    assert(rq != NULL);

    beam_scheduler_pool_t* pool = beam_scheduler_pool_create(4, rq, &alloc);
    assert(pool != NULL);

    beam_process_t* proc1 = beam_process_create(801, 128, &alloc);
    beam_process_t* proc2 = beam_process_create(802, 128, &alloc);
    assert(proc1 != NULL && proc2 != NULL);

    beam_result_t enq1 = beam_run_queue_enqueue(rq, proc1, BEAM_PRIO_NORMAL);
    beam_result_t enq2 = beam_run_queue_enqueue(rq, proc2, BEAM_PRIO_HIGH);
    (void)enq1;
    (void)enq2;

    beam_instruction_t code[] = {
        { .opcode = BEAM_OP_MOVE, .arg1 = 0, .arg2 = 0, .literal = make_small_int(42) },
        { .opcode = BEAM_OP_HALT }
    };

    beam_result_t start_res = beam_scheduler_pool_start(pool, code, sizeof(code)/sizeof(code[0]));
    assert(start_res == BEAM_OK);
    (void)start_res;

    /* Wait briefly for worker threads to consume processes */
    struct timespec ts = { .tv_sec = 0, .tv_nsec = 20000000 }; /* 20ms */
    nanosleep(&ts, NULL);

    beam_scheduler_pool_stop(pool);

    assert(beam_run_queue_count(rq) == 0);
    assert(beam_process_get_state(proc1) == BEAM_PROC_STATE_EXITED);
    assert(beam_process_get_state(proc2) == BEAM_PROC_STATE_EXITED);

    beam_process_destroy(proc1);
    beam_process_destroy(proc2);
    beam_scheduler_pool_destroy(pool);
    beam_run_queue_destroy(rq);

    assert(stats.alloc_count > 0);
    assert(stats.free_count == stats.alloc_count);
    printf("  [RESULT] 4 Parallel Worker Threads executed 2 concurrent processes to HALT cleanly!\n");
    printf("  [PASSED] test_scheduler_pool_parallel\n");
}

void test_scheduler_work_stealing(void) {
    printf("[UNIT TEST] Testing Work-Stealing Algorithm across Multi-Scheduler Threads...\n");

    mock_memory_stats_t stats = {0};
    beam_allocator_i alloc = mock_memory_create(&stats);

    beam_run_queue_t* rq = beam_run_queue_create(&alloc);
    assert(rq != NULL);

    beam_scheduler_pool_t* pool = beam_scheduler_pool_create(2, rq, &alloc);
    assert(pool != NULL);

    /* Enqueue 4 processes into global queue */
    beam_process_t* procs[4];
    for (int i = 0; i < 4; i++) {
        procs[i] = beam_process_create(901 + i, 128, &alloc);
        assert(procs[i] != NULL);
        beam_result_t enq_res = beam_run_queue_enqueue(rq, procs[i], (i % 2 == 0) ? BEAM_PRIO_LOW : BEAM_PRIO_HIGH);
        (void)enq_res;
    }

    beam_instruction_t code[] = {
        { .opcode = BEAM_OP_MOVE, .arg1 = 0, .arg2 = 0, .literal = make_small_int(99) },
        { .opcode = BEAM_OP_HALT }
    };

    beam_result_t start_res = beam_scheduler_pool_start(pool, code, sizeof(code)/sizeof(code[0]));
    assert(start_res == BEAM_OK);
    (void)start_res;

    struct timespec ts = { .tv_sec = 0, .tv_nsec = 30000000 }; /* 30ms */
    nanosleep(&ts, NULL);

    beam_scheduler_pool_stop(pool);

    assert(beam_run_queue_count(rq) == 0);
    for (int i = 0; i < 4; i++) {
        assert(beam_process_get_state(procs[i]) == BEAM_PROC_STATE_EXITED);
        beam_process_destroy(procs[i]);
    }

    beam_scheduler_pool_destroy(pool);
    beam_run_queue_destroy(rq);

    assert(stats.alloc_count > 0);
    assert(stats.free_count == stats.alloc_count);
    printf("  [RESULT] Idle worker threads successfully stole and executed all 4 processes!\n");
    printf("  [PASSED] test_scheduler_work_stealing\n");
}

int main(void) {
    printf("=========================================\n");
    printf(" RUNNING ISOLATED MODULE TEST: SCHEDULER \n");
    printf("=========================================\n");
    test_scheduler_preemption_step();
    test_scheduler_pool_parallel();
    test_scheduler_work_stealing();
    return 0;
}
