#define _POSIX_C_SOURCE 199309L
#include "beam_scheduler_loop_internal.h"
#include "beam_emu_internal.h"
#include <string.h>
#include <time.h>

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
        beam_result_t enq_res = beam_run_queue_enqueue(sched->run_queue, proc, BEAM_PRIO_NORMAL);
        (void)enq_res;
    }

    return res;
}

typedef struct {
    beam_scheduler_pool_t* pool;
    beam_scheduler_t* sched;
} worker_arg_t;

static void* scheduler_worker_thread(void* arg) {
    worker_arg_t* warg = (worker_arg_t*)arg;
    beam_scheduler_pool_t* pool = warg->pool;
    beam_scheduler_t* sched = warg->sched;
    beam_allocator_i alloc = pool->alloc;

    alloc.free(alloc.ctx, warg);

    while (pool->running) {
        beam_result_t res = beam_scheduler_step(sched, pool->code, pool->code_len);
        if (res == BEAM_ERR_NOT_FOUND) {
            /* No runnable process currently in queue: yield thread CPU time */
            struct timespec ts = { .tv_sec = 0, .tv_nsec = 1000000 }; /* 1ms */
            nanosleep(&ts, NULL);
        }
    }

    return NULL;
}

beam_scheduler_pool_t* beam_scheduler_pool_create(uint32_t num_workers, beam_run_queue_t* rq, const beam_allocator_i* alloc) {
    if (num_workers == 0 || !rq || !alloc || !alloc->alloc || !alloc->free) return NULL;

    beam_scheduler_pool_t* pool = (beam_scheduler_pool_t*)alloc->alloc(alloc->ctx, sizeof(beam_scheduler_pool_t));
    if (!pool) return NULL;

    memset(pool, 0, sizeof(beam_scheduler_pool_t));
    pool->num_workers = num_workers;
    pool->run_queue = rq;
    pool->alloc = *alloc;

    pool->schedulers = (beam_scheduler_t**)alloc->alloc(alloc->ctx, sizeof(beam_scheduler_t*) * num_workers);
    pool->threads = (pthread_t*)alloc->alloc(alloc->ctx, sizeof(pthread_t) * num_workers);

    if (!pool->schedulers || !pool->threads) {
        beam_scheduler_pool_destroy(pool);
        return NULL;
    }

    for (uint32_t i = 0; i < num_workers; i++) {
        pool->schedulers[i] = beam_scheduler_create(i + 1, rq, alloc);
        if (!pool->schedulers[i]) {
            beam_scheduler_pool_destroy(pool);
            return NULL;
        }
    }

    return pool;
}

void beam_scheduler_pool_destroy(beam_scheduler_pool_t* pool) {
    if (!pool) return;
    beam_allocator_i alloc = pool->alloc;

    if (pool->running) {
        beam_scheduler_pool_stop(pool);
    }

    if (pool->schedulers) {
        for (uint32_t i = 0; i < pool->num_workers; i++) {
            if (pool->schedulers[i]) {
                beam_scheduler_destroy(pool->schedulers[i]);
            }
        }
        alloc.free(alloc.ctx, pool->schedulers);
    }

    if (pool->threads) {
        alloc.free(alloc.ctx, pool->threads);
    }

    alloc.free(alloc.ctx, pool);
}

beam_result_t beam_scheduler_pool_start(beam_scheduler_pool_t* pool, const beam_instruction_t* code, size_t code_len) {
    if (!pool || pool->running) return BEAM_ERR_INVALID_ARG;

    pool->code = code;
    pool->code_len = code_len;
    pool->running = true;

    for (uint32_t i = 0; i < pool->num_workers; i++) {
        worker_arg_t* warg = (worker_arg_t*)pool->alloc.alloc(pool->alloc.ctx, sizeof(worker_arg_t));
        if (!warg) {
            pool->running = false;
            return BEAM_ERR_NO_MEMORY;
        }
        warg->pool = pool;
        warg->sched = pool->schedulers[i];

        if (pthread_create(&pool->threads[i], NULL, scheduler_worker_thread, warg) != 0) {
            pool->alloc.free(pool->alloc.ctx, warg);
            pool->running = false;
            return BEAM_ERR_INVALID_ARG;
        }
    }

    return BEAM_OK;
}

void beam_scheduler_pool_stop(beam_scheduler_pool_t* pool) {
    if (!pool || !pool->running) return;

    pool->running = false;

    for (uint32_t i = 0; i < pool->num_workers; i++) {
        pthread_join(pool->threads[i], NULL);
    }
}
