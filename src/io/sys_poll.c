#include "sys_poll_internal.h"
#include "beam_messaging.h"
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

#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>

int beam_socket_listen(uint16_t port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }

    if (listen(fd, 128) < 0) {
        close(fd);
        return -1;
    }

    /* Set non-blocking mode */
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    return fd;
}

int beam_socket_accept(int server_fd) {
    if (server_fd < 0) return -1;

    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
    if (client_fd >= 0) {
        int flags = fcntl(client_fd, F_GETFL, 0);
        fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
    }
    return client_fd;
}

beam_result_t beam_socket_dispatch_mailbox(beam_process_t* proc, int client_fd, const beam_allocator_i* alloc) {
    if (!proc || client_fd < 0) return BEAM_ERR_INVALID_ARG;

    char buf[256];
    ssize_t n = read(client_fd, buf, sizeof(buf) - 1);
    if (n <= 0) return BEAM_ERR_NOT_FOUND;

    buf[n] = '\0';
    Eterm msg_val = make_small_int((intptr_t)n);
    return beam_message_send_to_process(proc, msg_val, alloc);
}
