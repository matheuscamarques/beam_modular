#ifndef ERL_PROCESS_INTERNAL_H
#define ERL_PROCESS_INTERNAL_H

#include "beam_scheduler.h"
#include "beam_messaging.h"
#include "../emulator/beam_emu_internal.h"

#define BEAM_DEFAULT_REDUCTIONS 4000

struct beam_process {
    uint32_t pid;
    beam_process_state_t state;
    beam_priority_t priority;
    int reductions;
    
    /* Private Heap & Dynamic Unified Stack */
    Eterm* heap;
    size_t heap_capacity;
    size_t heap_top;
    size_t stack_top; /* Unified stack grows downward from heap_capacity */
    
    /* Mailbox */
    beam_mailbox_t* mailbox;

    /* Execution Frame */
    beam_emulator_frame_t frame;

    beam_allocator_i alloc;
};

/* Unified Stack/Heap Allocation Helpers */
BEAM_NODISCARD beam_result_t beam_process_stack_push(beam_process_t* proc, Eterm term);
BEAM_NODISCARD beam_result_t beam_process_stack_pop(beam_process_t* proc, Eterm* out_term);

#endif /* ERL_PROCESS_INTERNAL_H */
