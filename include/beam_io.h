#ifndef BEAM_IO_H
#define BEAM_IO_H

#include <stdarg.h>
#include <stdbool.h>

#include "beam_core.h"
#include "beam_memory.h"
#include "beam_scheduler.h"

/**
 * @file beam_io.h
 * @brief Public Opaque Interface for I/O Subsystem, Drivers, and NIFs.
 */

typedef struct beam_io_poller beam_io_poller_t;
typedef struct ErlNifEnv ErlNifEnv;
typedef Eterm ERL_NIF_TERM;

/* Event flags for Poller */
#define BEAM_IO_READABLE 0x1
#define BEAM_IO_WRITABLE 0x2

/* Driver Poller Interface */
beam_io_poller_t* beam_io_poller_create(const beam_allocator_i* alloc);
void beam_io_poller_destroy(beam_io_poller_t* poller);

beam_result_t beam_io_register_fd(beam_io_poller_t* poller, int fd, uint32_t events);
beam_result_t beam_io_poll(beam_io_poller_t* poller, int timeout_ms, int* out_events_ready);

/* Native Implemented Functions (NIF) Public API */
ErlNifEnv* enif_alloc_env(const beam_allocator_i* alloc, beam_process_t* proc);
void enif_free_env(ErlNifEnv* env);

ERL_NIF_TERM enif_make_int(ErlNifEnv* env, int val);
bool enif_get_int(ErlNifEnv* env, ERL_NIF_TERM term, int* out_val);
ERL_NIF_TERM enif_make_tuple_from_array(ErlNifEnv* env, const ERL_NIF_TERM* elements, unsigned int cnt);

#endif /* BEAM_IO_H */
