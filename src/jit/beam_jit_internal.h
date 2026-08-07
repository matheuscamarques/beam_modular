#ifndef BEAM_JIT_INTERNAL_H
#define BEAM_JIT_INTERNAL_H

#include "beam_jit.h"
#include <sys/mman.h>

struct beam_jit_engine {
    size_t total_compiled_bytes;
    beam_allocator_i alloc;
};

#endif /* BEAM_JIT_INTERNAL_H */
