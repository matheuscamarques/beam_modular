#ifndef BEAM_IO_H
#define BEAM_IO_H

#include "beam_core.h"
#include "beam_memory.h"

/**
 * @file beam_io.h
 * @brief Public Opaque Interface for I/O Subsystem, NIFs, and Driver Polling.
 */

typedef struct beam_io_poller beam_io_poller_t;
typedef struct beam_nif_env beam_nif_env_t;

/* Event flags for Poller */
#define BEAM_IO_READABLE 0x1
#define BEAM_IO_WRITABLE 0x2

beam_io_poller_t* beam_io_poller_create(const beam_allocator_i* alloc);
void beam_io_poller_destroy(beam_io_poller_t* poller);

beam_result_t beam_io_register_fd(beam_io_poller_t* poller, int fd, uint32_t events);
beam_result_t beam_io_poll(beam_io_poller_t* poller, int timeout_ms, int* out_events_ready);

#endif /* BEAM_IO_H */
