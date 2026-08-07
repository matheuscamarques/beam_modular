#ifndef BEAM_GC_INTERNAL_H
#define BEAM_GC_INTERNAL_H

#include "beam_memory.h"
#include "beam_scheduler.h"

#include <stdint.h>

typedef struct {
    size_t gc_count;
    size_t reclaimed_bytes;
} beam_gc_stats_t;

/* Off-heap reference-counted binary payload (ProcBin). The refcount tracks
 * the number of live process references: reaching zero frees the payload. */
typedef struct beam_procbin_payload {
    uint32_t refcount;
    uint32_t reserved;
    size_t   size;          /* payload byte length */
    uint8_t  data[];        /* binary payload bytes (flexible array member) */
} beam_procbin_payload_t;

/* Process-scoped registry of off-heap payloads so the GC can release and
 * destroy payloads that became unreachable from process roots. */
typedef struct beam_procbin_registry {
    beam_procbin_payload_t** slots;
    size_t count;
    size_t capacity;
} beam_procbin_registry_t;

/* Forwarding cache: maps a from-space object address to its new (to-space)
 * home during a copying collection. */
typedef struct {
    const Eterm* from;
    Eterm        to;
} beam_forward_t;

#define BEAM_GC_HEAP_GROWTH_WORDS 64u

#endif /* BEAM_GC_INTERNAL_H */
