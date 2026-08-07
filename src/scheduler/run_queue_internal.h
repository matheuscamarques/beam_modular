#ifndef RUN_QUEUE_INTERNAL_H
#define RUN_QUEUE_INTERNAL_H

#include "beam_scheduler.h"

#include <pthread.h>

typedef struct run_queue_node {
    beam_process_t* proc;
    struct run_queue_node* next;
} run_queue_node_t;

typedef struct {
    run_queue_node_t* head;
    run_queue_node_t* tail;
    size_t count;
} priority_level_queue_t;

struct beam_run_queue {
    priority_level_queue_t queues[BEAM_NUM_PRIORITIES];
    size_t total_count;
    pthread_mutex_t lock;
    beam_allocator_i alloc;
};

#endif /* RUN_QUEUE_INTERNAL_H */
