#include "dist_node_internal.h"
#include <string.h>

beam_node_table_t* beam_node_table_create(const beam_allocator_i* alloc) {
    if (!alloc || !alloc->alloc || !alloc->free) return NULL;

    beam_node_table_t* nt = (beam_node_table_t*)alloc->alloc(alloc->ctx, sizeof(beam_node_table_t));
    if (!nt) return NULL;

    memset(nt, 0, sizeof(beam_node_table_t));
    nt->alloc = *alloc;
    return nt;
}

void beam_node_table_destroy(beam_node_table_t* nt) {
    if (!nt) return;
    beam_allocator_i alloc = nt->alloc;
    alloc.free(alloc.ctx, nt);
}

beam_result_t beam_node_table_connect(beam_node_table_t* nt, const char* node_name) {
    if (!nt || !node_name) return BEAM_ERR_INVALID_ARG;

    /* Check if already connected */
    for (size_t i = 0; i < nt->count; i++) {
        if (strcmp(nt->nodes[i].name, node_name) == 0) {
            nt->nodes[i].connected = true;
            return BEAM_OK;
        }
    }

    if (nt->count >= MAX_DIST_NODES) return BEAM_ERR_NO_MEMORY;

    strncpy(nt->nodes[nt->count].name, node_name, sizeof(nt->nodes[nt->count].name) - 1);
    nt->nodes[nt->count].connected = true;
    nt->count++;

    return BEAM_OK;
}

bool beam_node_table_is_connected(const beam_node_table_t* nt, const char* node_name) {
    if (!nt || !node_name) return false;

    for (size_t i = 0; i < nt->count; i++) {
        if (strcmp(nt->nodes[i].name, node_name) == 0) {
            return nt->nodes[i].connected;
        }
    }

    return false;
}

size_t beam_node_table_count(const beam_node_table_t* nt) {
    return nt ? nt->count : 0;
}
