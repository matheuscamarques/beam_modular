#ifndef ERL_PROCESS_INTERNAL_H
#define ERL_PROCESS_INTERNAL_H

#include "beam_scheduler.h"
#include "beam_messaging.h"

#define BEAM_DEFAULT_REDUCTIONS 4000

struct beam_process {
    uint32_t pid;
    beam_process_state_t state;
    int reductions;
    
    /* Private Heap */
    Eterm* heap;
    size_t heap_capacity;
    size_t heap_top;
    
    /* Mailbox */
    beam_mailbox_t* mailbox;

    beam_allocator_i alloc;
};

#endif /* ERL_PROCESS_INTERNAL_H */
