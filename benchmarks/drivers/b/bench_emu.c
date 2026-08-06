#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>

#include "beam_core.h"
#include "beam_memory.h"
#include "beam_scheduler.h"
#include "beam_emu_internal.h"

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
    long N = (argc > 1) ? atol(argv[1]) : 10000000;
    if (N < 0) { fprintf(stderr, "invalid N\n"); return 2; }

    beam_allocator_i alloc = beam_allocator_create_system();
    beam_process_t* proc = beam_process_create(1, 128, &alloc);
    if (!proc) { fprintf(stderr, "process create failed\n"); return 1; }

    /* Loop X0 += X1(X1=1): linear program with N ADD operations.
     * The interpreter writes *out_result ONLY when the PC falls outside the
     * array (reduction exhaustion returns BEAM_OK WITHOUT writing the result),
     * so we use inline instructions and reductions = N + 2: exactly N ADDs
     * executed, natural exit (X0 = N). */
    size_t nitems = (size_t)N + 1; /* 1 MOVE + N ADD */
    beam_instruction_t* code = (beam_instruction_t*)malloc(nitems * sizeof(beam_instruction_t));
    if (!code) { fprintf(stderr, "malloc failed\n"); return 1; }

    code[0] = (beam_instruction_t){ .opcode = BEAM_OP_MOVE, .arg2 = 1, .literal = make_small_int(1) };
    for (size_t i = 1; i < nitems; i++) {
        code[i] = (beam_instruction_t){ .opcode = BEAM_OP_ADD, .arg1 = 0, .arg2 = 1, .arg3 = 0 };
    }
    beam_process_set_reductions(proc, (int)N + 2);

    char line[64];
    Eterm result = 0;

    long long t0 = monotonic_us();
    beam_result_t res = beam_emu_execute_code(proc, code, nitems, &result);
    long long t1 = monotonic_us();

    if (res != BEAM_OK && res != BEAM_ERR_HALT) {
        fprintf(stderr, "execution failed (res=%d)\n", (int)res);
        free(code);
        return 1;
    }

    long long value = (long long)eterm_to_small_int(result);

    snprintf(line, sizeof(line), "value=%lld", value);
    emit_line(line);

    finish(t1 - t0, N);

    printf("METRIC reductions=%d\n", beam_process_get_reductions(proc));
    printf("METRIC estado=%d\n", (int)beam_process_get_state(proc));

    free(code);
    beam_process_destroy(proc);
    return 0;
}