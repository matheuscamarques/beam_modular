#define _GNU_SOURCE
#include "dist_node_internal.h"
#include <string.h>

beam_node_table_t* beam_node_table_create(const beam_allocator_i* alloc) {
    if (!alloc || !alloc->alloc || !alloc->free) return NULL;

    beam_node_table_t* nt = (beam_node_table_t*)alloc->alloc(alloc->ctx, sizeof(beam_node_table_t));
    if (!nt) return NULL;

    memset(nt, 0, sizeof(beam_node_table_t));
    nt->alloc = *alloc;
    pthread_rwlock_init(&nt->rwlock, NULL);
    return nt;
}

void beam_node_table_destroy(beam_node_table_t* nt) {
    if (!nt) return;
    pthread_rwlock_destroy(&nt->rwlock);
    beam_allocator_i alloc = nt->alloc;
    alloc.free(alloc.ctx, nt);
}

beam_result_t beam_node_table_connect_port(beam_node_table_t* nt, const char* node_name, uint16_t port) {
    if (!nt || !node_name) return BEAM_ERR_INVALID_ARG;

    pthread_rwlock_wrlock(&nt->rwlock);

    /* Check if already connected */
    for (size_t i = 0; i < nt->count; i++) {
        if (strcmp(nt->nodes[i].name, node_name) == 0) {
            nt->nodes[i].connected = true;
            nt->nodes[i].port = port;
            pthread_rwlock_unlock(&nt->rwlock);
            return BEAM_OK;
        }
    }

    if (nt->count >= MAX_DIST_NODES) {
        pthread_rwlock_unlock(&nt->rwlock);
        return BEAM_ERR_NO_MEMORY;
    }

    strncpy(nt->nodes[nt->count].name, node_name, sizeof(nt->nodes[nt->count].name) - 1);
    nt->nodes[nt->count].connected = true;
    nt->nodes[nt->count].port = port;
    nt->count++;

    pthread_rwlock_unlock(&nt->rwlock);
    return BEAM_OK;
}

beam_result_t beam_node_table_connect(beam_node_table_t* nt, const char* node_name) {
    return beam_node_table_connect_port(nt, node_name, 4369); /* Default Erlang EPMD Port */
}

beam_result_t beam_node_epmd_register(const char* node_name, uint16_t port) {
    if (!node_name || port == 0) return BEAM_ERR_INVALID_ARG;
    /* Simulated Erlang Port Mapper Daemon (EPMD) handshake registration */
    return BEAM_OK;
}

bool beam_node_table_is_connected(const beam_node_table_t* nt, const char* node_name) {
    if (!nt || !node_name) return false;

    beam_node_table_t* non_const_nt = (beam_node_table_t*)nt;
    pthread_rwlock_rdlock(&non_const_nt->rwlock);

    for (size_t i = 0; i < nt->count; i++) {
        if (strcmp(nt->nodes[i].name, node_name) == 0) {
            bool conn = nt->nodes[i].connected;
            pthread_rwlock_unlock(&non_const_nt->rwlock);
            return conn;
        }
    }

    pthread_rwlock_unlock(&non_const_nt->rwlock);
    return false;
}

size_t beam_node_table_count(const beam_node_table_t* nt) {
    if (!nt) return 0;
    beam_node_table_t* non_const_nt = (beam_node_table_t*)nt;
    pthread_rwlock_rdlock(&non_const_nt->rwlock);
    size_t cnt = nt->count;
    pthread_rwlock_unlock(&non_const_nt->rwlock);
    return cnt;
}
