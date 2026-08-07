#ifndef BEAM_SCHEDULER_LOOP_INTERNAL_H
#define BEAM_SCHEDULER_LOOP_INTERNAL_H

#include "beam_scheduler.h"

#include <pthread.h>

struct beam_scheduler {
    uint32_t id;
    beam_run_queue_t* run_queue;
    beam_allocator_i alloc;
};

struct beam_scheduler_pool {
    uint32_t num_workers;
    beam_scheduler_t** schedulers;
    pthread_t* threads;
    beam_run_queue_t* run_queue;
    const beam_instruction_t* code;
    size_t code_len;
    volatile bool running;
    beam_allocator_i alloc;
};

#endif /* BEAM_SCHEDULER_LOOP_INTERNAL_H */
