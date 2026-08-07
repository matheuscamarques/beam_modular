#ifndef BEAM_SCHEDULER_H
#define BEAM_SCHEDULER_H

#include "beam_core.h"
#include "beam_memory.h"

/**
 * @file beam_scheduler.h
 * @brief Public Opaque Interface for Scheduler, Priority Run Queues, and PCB (C23 ISO Standard).
 */

typedef struct beam_run_queue beam_run_queue_t;
typedef struct beam_scheduler beam_scheduler_t;
typedef struct beam_mailbox beam_mailbox_t;
typedef struct beam_instruction beam_instruction_t;

#define BEAM_DEFAULT_REDUCTIONS 4000

typedef enum {
    BEAM_PRIO_MAX = 0,
    BEAM_PRIO_HIGH,
    BEAM_PRIO_NORMAL,
    BEAM_PRIO_LOW,
    BEAM_PRIO_NUM_LEVELS
} beam_priority_t;

#define BEAM_NUM_PRIORITIES BEAM_PRIO_NUM_LEVELS

typedef enum {
    BEAM_PROC_STATE_RUNNABLE = 0,
    BEAM_PROC_STATE_RUNNING,
    BEAM_PROC_STATE_WAITING,
    BEAM_PROC_STATE_SUSPENDED,
    BEAM_PROC_STATE_EXITED
} beam_process_state_t;

/* Process Control Block (PCB) Operations */
BEAM_NODISCARD beam_process_t* beam_process_create(uint32_t pid, size_t initial_heap_size, const beam_allocator_i* alloc);
void beam_process_destroy(beam_process_t* proc);

uint32_t beam_process_get_pid(const beam_process_t* proc);
beam_process_state_t beam_process_get_state(const beam_process_t* proc);
void beam_process_set_state(beam_process_t* proc, beam_process_state_t state);

beam_priority_t beam_process_get_priority(const beam_process_t* proc);
void beam_process_set_priority(beam_process_t* proc, beam_priority_t prio);

int beam_process_get_reductions(const beam_process_t* proc);
void beam_process_set_reductions(beam_process_t* proc, int reductions);
void beam_process_consume_reductions(beam_process_t* proc, int count);
void beam_process_reset_reductions(beam_process_t* proc);

size_t beam_process_heap_used(const beam_process_t* proc);
size_t beam_process_heap_capacity(const beam_process_t* proc);
BEAM_NODISCARD Eterm* beam_process_alloc_heap(beam_process_t* proc, size_t size);

beam_mailbox_t* beam_process_get_mailbox(beam_process_t* proc);
BEAM_NODISCARD beam_result_t beam_process_receive_message(beam_process_t* proc, Eterm* out_msg);

/* Priority Run Queue Operations */
BEAM_NODISCARD beam_run_queue_t* beam_run_queue_create(const beam_allocator_i* alloc);
void beam_run_queue_destroy(beam_run_queue_t* rq);

BEAM_NODISCARD beam_result_t beam_run_queue_enqueue(beam_run_queue_t* rq, beam_process_t* proc, beam_priority_t prio);
BEAM_NODISCARD beam_process_t* beam_run_queue_dequeue(beam_run_queue_t* rq);
BEAM_NODISCARD beam_process_t* beam_run_queue_steal(beam_run_queue_t* rq);
size_t beam_run_queue_count(const beam_run_queue_t* rq);

/* Preemption Scheduler Engine */
BEAM_NODISCARD beam_scheduler_t* beam_scheduler_create(uint32_t scheduler_id, beam_run_queue_t* rq, const beam_allocator_i* alloc);
void beam_scheduler_destroy(beam_scheduler_t* sched);
BEAM_NODISCARD beam_result_t beam_scheduler_step(beam_scheduler_t* sched, const beam_instruction_t* code, size_t code_len);

/* Multi-Scheduler Thread Pool (SMP) */
typedef struct beam_scheduler_pool beam_scheduler_pool_t;

BEAM_NODISCARD beam_scheduler_pool_t* beam_scheduler_pool_create(uint32_t num_workers, beam_run_queue_t* rq, const beam_allocator_i* alloc);
void beam_scheduler_pool_destroy(beam_scheduler_pool_t* pool);
BEAM_NODISCARD beam_result_t beam_scheduler_pool_start(beam_scheduler_pool_t* pool, const beam_instruction_t* code, size_t code_len);
void beam_scheduler_pool_stop(beam_scheduler_pool_t* pool);

#endif /* BEAM_SCHEDULER_H */
