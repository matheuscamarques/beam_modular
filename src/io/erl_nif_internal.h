#ifndef ERL_NIF_INTERNAL_H
#define ERL_NIF_INTERNAL_H

#include "beam_io.h"

struct ErlNifEnv {
    beam_process_t* proc;
    beam_allocator_i alloc;
};

#endif /* ERL_NIF_INTERNAL_H */
