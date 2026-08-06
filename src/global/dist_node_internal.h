#ifndef DIST_NODE_INTERNAL_H
#define DIST_NODE_INTERNAL_H

#include "beam_global.h"

#define MAX_DIST_NODES 64

typedef struct {
    char name[64];
    bool connected;
} dist_node_entry_t;

struct beam_node_table {
    dist_node_entry_t nodes[MAX_DIST_NODES];
    size_t count;
    beam_allocator_i alloc;
};

#endif /* DIST_NODE_INTERNAL_H */
