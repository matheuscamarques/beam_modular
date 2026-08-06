#ifndef BEAM_SCHEDULER_H
#define BEAM_SCHEDULER_H

#include "beam_core.h"
#include "beam_memory.h"

/**
 * @file beam_scheduler.h
 * @brief Public Opaque Interface for Processes (PCB) and Run Queues.
 */

typedef struct beam_process beam_process_t;
typedef struct beam_run_queue beam_run_queue_t;
typedef struct beam_scheduler beam_scheduler_t;

typedef enum {
    BEAM_PROC_STATE_RUNNABLE,
    BEAM_PROC_STATE_RUNNING,
    BEAM_PROC_STATE_WAITING,
    BEAM_PROC_STATE_EXITED
} beam_process_state_t;

/* Process Lifecycle & Memory */
beam_process_t* beam_process_create(uint32_t pid, size_t initial_heap_words, const beam_allocator_i* alloc);
void beam_process_destroy(beam_process_t* proc);

/* Process Accessors */
uint32_t beam_process_get_pid(const beam_process_t* proc);
beam_process_state_t beam_process_get_state(const beam_process_t* proc);
void beam_process_set_state(beam_process_t* proc, beam_process_state_t state);
int beam_process_get_reductions(const beam_process_t* proc);
void beam_process_set_reductions(beam_process_t* proc, int reductions);
void beam_process_consume_reductions(beam_process_t* proc, int count);

/* Heap allocation on Process Private Heap */
Eterm* beam_process_alloc_heap(beam_process_t* proc, size_t needed_words);
size_t beam_process_heap_used(const beam_process_t* proc);
size_t beam_process_heap_capacity(const beam_process_t* proc);

#endif /* BEAM_SCHEDULER_H */
