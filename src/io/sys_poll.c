#include "sys_poll_internal.h"
#include <string.h>

beam_io_poller_t* beam_io_poller_create(const beam_allocator_i* alloc) {
    if (!alloc || !alloc->alloc || !alloc->free) return NULL;

    beam_io_poller_t* poller = (beam_io_poller_t*)alloc->alloc(alloc->ctx, sizeof(beam_io_poller_t));
    if (!poller) return NULL;

    memset(poller, 0, sizeof(beam_io_poller_t));
    poller->alloc = *alloc;
    return poller;
}

void beam_io_poller_destroy(beam_io_poller_t* poller) {
    if (!poller) return;
    beam_allocator_i alloc = poller->alloc;
    alloc.free(alloc.ctx, poller);
}

beam_result_t beam_io_register_fd(beam_io_poller_t* poller, int fd, uint32_t events) {
    if (!poller || fd < 0) return BEAM_ERR_INVALID_ARG;
    if (poller->count >= MAX_POLL_ENTRIES) return BEAM_ERR_NO_MEMORY;

    poller->entries[poller->count].fd = fd;
    poller->entries[poller->count].events = events;
    poller->count++;

    return BEAM_OK;
}

beam_result_t beam_io_poll(beam_io_poller_t* poller, int timeout_ms, int* out_events_ready) {
    if (!poller || !out_events_ready) return BEAM_ERR_INVALID_ARG;
    (void)timeout_ms;

    /* Non-blocking poll simulation for registered descriptors */
    *out_events_ready = (int)poller->count;
    return BEAM_OK;
}
