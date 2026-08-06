#ifndef BEAM_MAIN_INTERNAL_H
#define BEAM_MAIN_INTERNAL_H

#include "beam_core.h"
#include "beam_memory.h"
#include "beam_scheduler.h"
#include "beam_messaging.h"
#include "beam_global.h"
#include "beam_io.h"

struct beam_context {
    beam_code_server_t* code_server;
    beam_atom_table_t* atom_table;
    beam_ets_table_t* ets_table;
    beam_io_poller_t* poller;
    beam_run_queue_t* run_queue;
    beam_scheduler_t* scheduler;
    beam_allocator_i alloc;
};

#endif /* BEAM_MAIN_INTERNAL_H */
