#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>

#include "beam_core.h"
#include "beam_memory.h"
#include "beam_global.h"

/* --- Common protocol --- */
static uint64_t g_fnv = 0xcbf29ce484222325ULL;

static void fnv_update(const unsigned char* data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        g_fnv ^= data[i];
        g_fnv *= 0x100000001b3ULL;
    }
}

static void emit_line(const char* line) {
    printf("RESULT %s\n", line);
    fnv_update((const unsigned char*)line, strlen(line));
    fnv_update((const unsigned char*)"\n", 1);
}

static void finish(long long time_us, long long ops) {
    printf("FINGERPRINT %016" PRIx64 "\n", g_fnv);
    printf("TIME_US %lld\n", time_us);
    printf("OPS %lld\n", ops);
}

static long long monotonic_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000LL + ts.tv_nsec / 1000;
}

int main(int argc, char** argv) {
    long N = (argc > 1) ? atol(argv[1]) : 50000;
    if (N < 0) { fprintf(stderr, "invalid N\n"); return 2; }

    beam_allocator_i alloc = beam_allocator_create_system();
    beam_ets_table_t* table = beam_ets_table_create("bench_ets", &alloc);
    if (!table) { fprintf(stderr, "ets create failed\n"); return 1; }

    char line[64];
    long long t0 = monotonic_us();

    /* Insert N keys */
    for (long i = 0; i < N; i++) {
        if (beam_ets_insert(table, make_small_int(i), make_small_int(i * 2)) != BEAM_OK) {
            fprintf(stderr, "insert failed at %ld\n", i);
            return 1;
        }
    }

    /* Lookup all */
    long found_all = 0;
    for (long i = 0; i < N; i++) {
        Eterm v;
        if (beam_ets_lookup(table, make_small_int(i), &v) == BEAM_OK) {
            found_all++;
        }
    }

    /* Delete evens (deterministic: same criterion as side A) */
    for (long i = 0; i < N; i += 2) {
        beam_result_t del_res = beam_ets_delete(table, make_small_int(i));
        (void)del_res;
    }

    /* Lookup remaining (odd) keys */
    long found_rest = 0;
    for (long i = 1; i < N; i += 2) {
        Eterm v;
        if (beam_ets_lookup(table, make_small_int(i), &v) == BEAM_OK) {
            found_rest++;
        }
    }

    size_t count = beam_ets_count(table);
    long long t1 = monotonic_us();

    snprintf(line, sizeof(line), "count=%zu", count);
    emit_line(line);
    snprintf(line, sizeof(line), "found_all=%ld", found_all);
    emit_line(line);
    snprintf(line, sizeof(line), "found_rest=%ld", found_rest);
    emit_line(line);

    finish(t1 - t0, 3 * N);

    beam_memory_stats_t st = beam_allocator_get_stats(&alloc);
    printf("METRIC total_allocated=%zu\n", st.total_allocated_bytes);
    printf("METRIC peak_allocated=%zu\n", st.peak_allocated_bytes);

    beam_ets_table_destroy(table);
    return 0;
}