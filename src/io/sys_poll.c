#include "sys_poll_internal.h"
#include <string.h>

#include <unistd.h>
#include <sys/epoll.h>

beam_io_poller_t* beam_io_poller_create(const beam_allocator_i* alloc) {
    if (!alloc || !alloc->alloc || !alloc->free) return NULL;

    beam_io_poller_t* poller = (beam_io_poller_t*)alloc->alloc(alloc->ctx, sizeof(beam_io_poller_t));
    if (!poller) return NULL;

    memset(poller, 0, sizeof(beam_io_poller_t));
    poller->alloc = *alloc;
    pthread_mutex_init(&poller->lock, NULL);

    poller->epoll_fd = epoll_create1(0);
    if (poller->epoll_fd < 0) {
        pthread_mutex_destroy(&poller->lock);
        alloc->free(alloc->ctx, poller);
        return NULL;
    }

    return poller;
}

void beam_io_poller_destroy(beam_io_poller_t* poller) {
    if (!poller) return;
    beam_allocator_i alloc = poller->alloc;

    if (poller->epoll_fd >= 0) {
        close(poller->epoll_fd);
    }
    pthread_mutex_destroy(&poller->lock);
    alloc.free(alloc.ctx, poller);
}

beam_result_t beam_io_poller_register(beam_io_poller_t* poller, int fd, uint32_t events) {
    if (!poller || fd < 0) return BEAM_ERR_INVALID_ARG;

    pthread_mutex_lock(&poller->lock);
    if (poller->count >= MAX_POLL_ENTRIES) {
        pthread_mutex_unlock(&poller->lock);
        return BEAM_ERR_NO_MEMORY;
    }

    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = 0;
    if (events & BEAM_IO_READABLE) ev.events |= EPOLLIN;
    if (events & BEAM_IO_WRITABLE) ev.events |= EPOLLOUT;
    if (events & BEAM_IO_ERROR)    ev.events |= EPOLLERR;
    ev.data.fd = fd;

    if (epoll_ctl(poller->epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        pthread_mutex_unlock(&poller->lock);
        return BEAM_ERR_INVALID_ARG;
    }

    poller->entries[poller->count].fd = fd;
    poller->entries[poller->count].events = events;
    poller->count++;
    pthread_mutex_unlock(&poller->lock);

    return BEAM_OK;
}

beam_result_t beam_io_poller_poll(beam_io_poller_t* poller, int timeout_ms, int* out_events_ready) {
    if (!poller || !out_events_ready) return BEAM_ERR_INVALID_ARG;

    struct epoll_event events[MAX_POLL_ENTRIES];
    int n = epoll_wait(poller->epoll_fd, events, MAX_POLL_ENTRIES, timeout_ms);
    if (n < 0) {
        *out_events_ready = 0;
        return BEAM_ERR_INVALID_ARG;
    }

    *out_events_ready = n;
    return BEAM_OK;
}
