#ifndef ETS_INTERNAL_H
#define ETS_INTERNAL_H

#include "beam_global.h"

#define ETS_DEFAULT_BUCKETS 64

typedef struct ets_entry {
    Eterm key;
    Eterm value;
    struct ets_entry* next;
} ets_entry_t;

struct beam_ets_table {
    char name[32];
    ets_entry_t** buckets;
    size_t bucket_count;
    size_t item_count;
    beam_allocator_i alloc;
};

#endif /* ETS_INTERNAL_H */
