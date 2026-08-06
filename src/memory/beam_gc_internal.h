#ifndef BEAM_GC_INTERNAL_H
#define BEAM_GC_INTERNAL_H

#include "beam_memory.h"
#include "beam_scheduler.h"

typedef struct {
    size_t gc_count;
    size_t reclaimed_bytes;
} beam_gc_stats_t;

#endif /* BEAM_GC_INTERNAL_H */
