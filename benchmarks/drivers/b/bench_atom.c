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

/* --- Common protocol: RESULT lines + FNV-1a 64 --- */
static uint64_t g_fnv = 0xcbf29ce484222325ULL;

static void emit_line(const char* line) {
    printf("RESULT %s\n", line);
    for (const unsigned char* p = (const unsigned char*)line; *p; p++) {
        g_fnv ^= *p;
        g_fnv *= 0x100000001b3ULL;
    }
    g_fnv ^= '\n';
    g_fnv *= 0x100000001b3ULL;
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

static int cmpstr(const void* a, const void* b) {
    return strcmp(*(const char**)a, *(const char**)b);
}

int main(int argc, char** argv) {
    long N = (argc > 1) ? atol(argv[1]) : 100000;
    if (N < 0) { fprintf(stderr, "invalid N\n"); return 2; }

    beam_allocator_i alloc = beam_allocator_create_system();
    beam_atom_table_t* table = beam_atom_table_create(&alloc, 0);
    if (!table) { fprintf(stderr, "atom table create failed\n"); return 1; }

    char name[32];
    long long t0 = monotonic_us();

    /* Insert phase */
    for (long i = 0; i < N; i++) {
        snprintf(name, sizeof(name), "a%ld", i);
        if (beam_atom_intern(table, name) == ETERM_INVALID) {
            fprintf(stderr, "intern failed at %ld\n", i);
            return 1;
        }
    }

    /* Collect + canonical sort (byte-for-byte, like Erlang lists:sort) */
    char** names = (char**)malloc((size_t)N * sizeof(char*));
    if (!names) { return 1; }
    for (long i = 0; i < N; i++) {
        snprintf(name, sizeof(name), "a%ld", i);
        Eterm term = beam_atom_intern(table, name);
        names[i] = (char*)beam_atom_lookup(table, term);
    }
    qsort(names, (size_t)N, sizeof(char*), cmpstr);

    /* Find phase (no insertion); names are null-terminated by the table */
    for (long i = 0; i < N; i++) {
        if (beam_atom_intern(table, names[i]) == ETERM_INVALID) {
            fprintf(stderr, "find failed\n");
            return 1;
        }
    }

    long long t1 = monotonic_us();

    for (long i = 0; i < N; i++) {
        emit_line(names[i]);
    }

    finish(t1 - t0, 2 * N);

    printf("METRIC atom_count=%ld\n", (long)beam_atom_table_count(table));
    beam_memory_stats_t st = beam_allocator_get_stats(&alloc);
    printf("METRIC total_allocated=%zu\n", st.total_allocated_bytes);
    printf("METRIC peak_allocated=%zu\n", st.peak_allocated_bytes);

    free(names);
    beam_atom_table_destroy(table);
    return 0;
}