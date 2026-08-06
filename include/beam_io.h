#ifndef BEAM_IO_H
#define BEAM_IO_H

#include "beam_core.h"
#include "beam_memory.h"

/**
 * @file beam_io.h
 * @brief Public Opaque Interface for Async Driver Event Polling and ErlNifEnv C-Extensions (C23 ISO Standard).
 */

typedef struct beam_io_poller beam_io_poller_t;
typedef struct ErlNifEnv ErlNifEnv;

typedef enum {
    BEAM_IO_READABLE = 0x01,
    BEAM_IO_WRITABLE = 0x02,
    BEAM_IO_ERROR    = 0x04
} beam_io_event_t;

/* Driver Poller Operations */
BEAM_NODISCARD beam_io_poller_t* beam_io_poller_create(const beam_allocator_i* alloc);
void beam_io_poller_destroy(beam_io_poller_t* poller);

BEAM_NODISCARD beam_result_t beam_io_poller_register(beam_io_poller_t* poller, int fd, uint32_t events_mask);
BEAM_NODISCARD beam_result_t beam_io_poller_poll(beam_io_poller_t* poller, int timeout_ms, int* out_ready_count);

/* NIF C-Extension Environment Operations */
BEAM_NODISCARD ErlNifEnv* enif_alloc_env(const beam_allocator_i* alloc, beam_process_t* proc);
void enif_free_env(ErlNifEnv* env);

BEAM_NODISCARD Eterm enif_make_int(ErlNifEnv* env, int val);
BEAM_NODISCARD bool enif_get_int(ErlNifEnv* env, Eterm term, int* out_val);
BEAM_NODISCARD Eterm enif_make_tuple_from_array(ErlNifEnv* env, const Eterm* elements, unsigned int cnt);

#endif /* BEAM_IO_H */
