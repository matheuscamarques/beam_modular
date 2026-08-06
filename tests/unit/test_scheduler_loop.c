#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

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
    beam_run_queue_enqueue(rq, proc, BEAM_PRIO_NORMAL);

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

int main(void) {
    printf("=========================================\n");
    printf(" RUNNING ISOLATED MODULE TEST: SCHEDULER \n");
    printf("=========================================\n");
    test_scheduler_preemption_step();
    return 0;
}
