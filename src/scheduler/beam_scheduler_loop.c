#include "beam_scheduler_loop_internal.h"
#include "beam_emu_internal.h"
#include <string.h>

beam_scheduler_t* beam_scheduler_create(uint32_t id, beam_run_queue_t* rq, const beam_allocator_i* alloc) {
    if (!rq || !alloc || !alloc->alloc || !alloc->free) return NULL;

    beam_scheduler_t* sched = (beam_scheduler_t*)alloc->alloc(alloc->ctx, sizeof(beam_scheduler_t));
    if (!sched) return NULL;

    memset(sched, 0, sizeof(beam_scheduler_t));
    sched->id = id;
    sched->run_queue = rq;
    sched->alloc = *alloc;
    return sched;
}

void beam_scheduler_destroy(beam_scheduler_t* sched) {
    if (!sched) return;
    beam_allocator_i alloc = sched->alloc;
    alloc.free(alloc.ctx, sched);
}

beam_result_t beam_scheduler_step(beam_scheduler_t* sched, const beam_instruction_t* code, size_t code_len) {
    if (!sched || !sched->run_queue) return BEAM_ERR_INVALID_ARG;

    beam_process_t* proc = beam_run_queue_dequeue(sched->run_queue);
    if (!proc) return BEAM_ERR_NOT_FOUND;

    Eterm out_val = 0;
    beam_result_t res = beam_emu_execute_code(proc, code, code_len, &out_val);
    (void)out_val;

    beam_process_state_t state = beam_process_get_state(proc);
    if (state == BEAM_PROC_STATE_RUNNABLE || (res == BEAM_OK && state == BEAM_PROC_STATE_RUNNING)) {
        /* Preempted due to reduction exhaustion: reset reductions and re-enqueue */
        beam_process_set_reductions(proc, BEAM_DEFAULT_REDUCTIONS);
        beam_run_queue_enqueue(sched->run_queue, proc, BEAM_PRIO_NORMAL);
    }

    return res;
}
