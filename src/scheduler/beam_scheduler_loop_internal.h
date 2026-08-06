#ifndef BEAM_SCHEDULER_LOOP_INTERNAL_H
#define BEAM_SCHEDULER_LOOP_INTERNAL_H

#include "beam_scheduler.h"

struct beam_scheduler {
    uint32_t id;
    beam_run_queue_t* run_queue;
    beam_allocator_i alloc;
};

#endif /* BEAM_SCHEDULER_LOOP_INTERNAL_H */
