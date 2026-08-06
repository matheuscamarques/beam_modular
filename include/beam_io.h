#ifndef BEAM_IO_H
#define BEAM_IO_H

#include "beam_core.h"

/**
 * @file beam_io.h
 * @brief Public Opaque Interface for I/O Subsystem, NIFs, and Driver Polling.
 */

typedef struct beam_io_poller beam_io_poller_t;
typedef struct beam_nif_env beam_nif_env_t;

beam_result_t beam_io_poll_events(beam_io_poller_t* poller, int timeout_ms);

#endif /* BEAM_IO_H */
