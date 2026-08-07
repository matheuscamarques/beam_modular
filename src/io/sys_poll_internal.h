#ifndef SYS_POLL_INTERNAL_H
#define SYS_POLL_INTERNAL_H

#include "beam_io.h"

#include <sys/epoll.h>
#include <pthread.h>

#define MAX_POLL_ENTRIES 64

typedef struct {
    int fd;
    uint32_t events;
} poll_entry_t;

struct beam_io_poller {
    int epoll_fd;
    poll_entry_t entries[MAX_POLL_ENTRIES];
    size_t count;
    pthread_mutex_t lock;
    beam_allocator_i alloc;
};

#endif /* SYS_POLL_INTERNAL_H */
