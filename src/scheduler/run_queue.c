#include "run_queue_internal.h"
#include <string.h>

beam_run_queue_t* beam_run_queue_create(const beam_allocator_i* alloc) {
    if (!alloc || !alloc->alloc || !alloc->free) return NULL;

    beam_run_queue_t* rq = (beam_run_queue_t*)alloc->alloc(alloc->ctx, sizeof(beam_run_queue_t));
    if (!rq) return NULL;

    memset(rq, 0, sizeof(beam_run_queue_t));
    rq->alloc = *alloc;
    pthread_mutex_init(&rq->lock, NULL);
    return rq;
}

void beam_run_queue_destroy(beam_run_queue_t* rq) {
    if (!rq) return;

    pthread_mutex_destroy(&rq->lock);
    beam_allocator_i alloc = rq->alloc;

    for (int p = 0; p < BEAM_NUM_PRIORITIES; p++) {
        run_queue_node_t* curr = rq->queues[p].head;
        while (curr) {
            run_queue_node_t* next = curr->next;
            alloc.free(alloc.ctx, curr);
            curr = next;
        }
    }

    alloc.free(alloc.ctx, rq);
}

beam_result_t beam_run_queue_enqueue(beam_run_queue_t* rq, beam_process_t* proc, beam_priority_t prio) {
    if (!rq || !proc || prio >= BEAM_NUM_PRIORITIES) return BEAM_ERR_INVALID_ARG;

    run_queue_node_t* node = (run_queue_node_t*)rq->alloc.alloc(rq->alloc.ctx, sizeof(run_queue_node_t));
    if (!node) return BEAM_ERR_NO_MEMORY;

    node->proc = proc;
    node->next = NULL;

    pthread_mutex_lock(&rq->lock);
    priority_level_queue_t* q = &rq->queues[prio];
    if (!q->tail) {
        q->head = node;
        q->tail = node;
    } else {
        q->tail->next = node;
        q->tail = node;
    }
    q->count++;
    rq->total_count++;
    pthread_mutex_unlock(&rq->lock);

    beam_process_set_state(proc, BEAM_PROC_STATE_RUNNABLE);
    return BEAM_OK;
}

beam_process_t* beam_run_queue_dequeue(beam_run_queue_t* rq) {
    if (!rq) return NULL;

    pthread_mutex_lock(&rq->lock);
    if (rq->total_count == 0) {
        pthread_mutex_unlock(&rq->lock);
        return NULL;
    }

    /* Dequeue highest priority process available (MAX -> HIGH -> NORMAL -> LOW) */
    for (int p = 0; p < BEAM_NUM_PRIORITIES; p++) {
        priority_level_queue_t* q = &rq->queues[p];
        if (q->head) {
            run_queue_node_t* node = q->head;
            beam_process_t* proc = node->proc;

            q->head = node->next;
            if (!q->head) {
                q->tail = NULL;
            }
            q->count--;
            rq->total_count--;
            pthread_mutex_unlock(&rq->lock);

            rq->alloc.free(rq->alloc.ctx, node);
            beam_process_set_state(proc, BEAM_PROC_STATE_RUNNING);
            return proc;
        }
    }

    pthread_mutex_unlock(&rq->lock);
    return NULL;
}

size_t beam_run_queue_count(const beam_run_queue_t* rq) {
    return rq ? rq->total_count : 0;
}
