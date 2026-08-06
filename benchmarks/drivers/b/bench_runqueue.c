#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>

#include "beam_core.h"
#include "beam_memory.h"
#include "beam_scheduler.h"

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

static beam_priority_t next_prio(long i) {
    static const beam_priority_t cycle[4] = {BEAM_PRIO_MAX, BEAM_PRIO_HIGH, BEAM_PRIO_NORMAL, BEAM_PRIO_LOW};
    return cycle[i % 4];
}

int main(int argc, char** argv) {
    long N = (argc > 1) ? atol(argv[1]) : 100000;
    if (N < 0) { fprintf(stderr, "invalid N\n"); return 2; }

    beam_allocator_i alloc = beam_allocator_create_system();
    beam_run_queue_t* rq = beam_run_queue_create(&alloc);
    if (!rq) { fprintf(stderr, "run queue create failed\n"); return 1; }

    beam_process_t** procs = (beam_process_t**)malloc((size_t)N * sizeof(beam_process_t*));
    if (!procs) { return 1; }

    char line[64];
    long long t0 = monotonic_us();

    /* Enqueue N processes with a priority mix */
    for (long i = 0; i < N; i++) {
        procs[i] = beam_process_create((uint32_t)(i + 1), 64, &alloc);
        if (!procs[i]) { fprintf(stderr, "process create failed at %ld\n", i); return 1; }
        if (beam_run_queue_enqueue(rq, procs[i], next_prio(i)) != BEAM_OK) {
            fprintf(stderr, "enqueue failed at %ld\n", i);
            return 1;
        }
    }

    /* Dequeue all */
    long dequeued = 0;
    beam_process_t* p;
    while ((p = beam_run_queue_dequeue(rq)) != NULL) {
        dequeued++;
    }

    long long t1 = monotonic_us();

    snprintf(line, sizeof(line), "enqueued=%ld", N);
    emit_line(line);
    snprintf(line, sizeof(line), "dequeued=%ld", dequeued);
    emit_line(line);

    finish(t1 - t0, 2 * N);

    printf("METRIC procs_created=%ld\n", N);
    beam_memory_stats_t st = beam_allocator_get_stats(&alloc);
    printf("METRIC total_allocated=%zu\n", st.total_allocated_bytes);

    for (long i = 0; i < N; i++) {
        if (procs[i]) beam_process_destroy(procs[i]);
    }
    free(procs);
    beam_run_queue_destroy(rq);
    return 0;
}