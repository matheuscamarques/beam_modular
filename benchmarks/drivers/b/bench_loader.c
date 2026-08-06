#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>

#include "beam_core.h"
#include "beam_memory.h"

/* --- Common protocol: RESULT lines + FNV-1a 64 + time --- */
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
    if (argc < 2) {
        fprintf(stderr, "usage: bench_loader <file.beam>\n");
        return 2;
    }

    FILE* f = fopen(argv[1], "rb");
    if (!f) { perror("fopen"); return 2; }
    fseek(f, 0, SEEK_END);
    long fsz = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t* buf = (uint8_t*)malloc((size_t)fsz + 1);
    if (!buf) { fclose(f); return 2; }
    size_t nread = fread(buf, 1, (size_t)fsz, f);
    fclose(f);
    if (nread != (size_t)fsz) { free(buf); return 2; }

    beam_allocator_i alloc = beam_allocator_create_system();

    long long t0 = monotonic_us();
    beam_file_t* beam = beam_file_parse(buf, nread, &alloc);
    long long t1 = monotonic_us();
    if (!beam) {
        fprintf(stderr, "beam_file_parse failed\n");
        free(buf);
        return 1;
    }

    char line[4096];
    const char* mod = beam_file_get_module_name(beam);
    if (!mod) { free(buf); return 1; }
    snprintf(line, sizeof(line), "module:%s", mod);
    emit_line(line);

    size_t acount = beam_file_get_atom_count(beam);
    for (size_t i = 0; i < acount; i++) {
        const char* an = beam_file_get_atom(beam, i);
        if (!an) { continue; }
        snprintf(line, sizeof(line), "atom:%s", an);
        emit_line(line);
    }

    finish(t1 - t0, (long long)acount);

    beam_file_destroy(beam);
    free(buf);
    return 0;
}