#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>

#include "beam_core.h"
#include "beam_memory.h"

/* --- Protocolo comum --- */
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
    long N = (argc > 1) ? atol(argv[1]) : 200000;
    if (N < 0) { fprintf(stderr, "N invalido\n"); return 2; }

    beam_allocator_i alloc = beam_allocator_create_system();
    char line[64];

    long long total_used = 0;
    long long t0 = monotonic_us();

    for (long i = 1; i <= N; i++) {
        size_t size = (size_t)((i % 5) * 16 + 4);
        void* p = alloc.alloc(alloc.ctx, size);
        if (!p) { fprintf(stderr, "alloc falhou em %ld\n", i); return 1; }
        memset(p, 1, size); /* touch todos os bytes */
        total_used += (long long)size;
        alloc.free(alloc.ctx, p);
    }

    long long t1 = monotonic_us();

    snprintf(line, sizeof(line), "ops=%ld", N);
    emit_line(line);
    snprintf(line, sizeof(line), "total_used=%lld", total_used);
    emit_line(line);

    finish(t1 - t0, N);

    beam_memory_stats_t st = beam_allocator_get_stats(&alloc);
    printf("METRIC total_allocated=%zu\n", st.total_allocated_bytes);
    printf("METRIC peak_allocated=%zu\n", st.peak_allocated_bytes);
    printf("METRIC active_allocations=%zu\n", st.active_allocations);

    return 0;
}