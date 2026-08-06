#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>

#include "beam_core.h"
#include "beam_memory.h"
#include "beam_messaging.h"

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
    long N = (argc > 1) ? atol(argv[1]) : 100000;
    if (N < 0) { fprintf(stderr, "invalid N\n"); return 2; }

    beam_allocator_i alloc = beam_allocator_create_system();
    beam_mailbox_t* mbox = beam_mailbox_create(&alloc);
    if (!mbox) { fprintf(stderr, "mailbox create failed\n"); return 1; }

    char line[64];
    long long t0 = monotonic_us();

    /* Enqueue N (send) */
    for (long i = 1; i <= N; i++) {
        if (beam_mailbox_enqueue(mbox, make_small_int(i)) != BEAM_OK) {
            fprintf(stderr, "enqueue failed at %ld\n", i);
            return 1;
        }
    }

    /* Dequeue N (receive) */
    long received = 0;
    for (long i = 1; i <= N; i++) {
        Eterm out;
        if (beam_mailbox_dequeue(mbox, &out) == BEAM_OK) {
            received++;
        }
    }

    long long t1 = monotonic_us();

    snprintf(line, sizeof(line), "sent=%ld", N);
    emit_line(line);
    snprintf(line, sizeof(line), "received=%ld", received);
    emit_line(line);

    finish(t1 - t0, 2 * N);

    printf("METRIC mailbox_remaining=%zu\n", beam_mailbox_count(mbox));
    beam_memory_stats_t st = beam_allocator_get_stats(&alloc);
    printf("METRIC total_allocated=%zu\n", st.total_allocated_bytes);

    beam_mailbox_destroy(mbox);
    return 0;
}