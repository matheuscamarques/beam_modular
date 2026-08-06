#include <stdio.h>
#include <stdlib.h>
#include "beam_core.h"
#include "beam_memory.h"
#include "beam_global.h"
#include "beam_scheduler.h"
#include "beam_messaging.h"

struct beam_context {
    beam_allocator_i system_alloc;
    beam_atom_table_t* atom_table;
    beam_ets_table_t* system_ets;
    beam_process_t* init_process;
};

beam_context_t* beam_context_init(void) {
    beam_allocator_i alloc = beam_allocator_create_system();

    beam_context_t* ctx = (beam_context_t*)alloc.alloc(alloc.ctx, sizeof(beam_context_t));
    if (!ctx) return NULL;

    ctx->system_alloc = alloc;

    /* Initialize Global Atom Table */
    ctx->atom_table = beam_atom_table_create(&ctx->system_alloc, 256);
    if (!ctx->atom_table) {
        beam_allocator_destroy(&ctx->system_alloc);
        return NULL;
    }

    /* Initialize Global ETS Database */
    ctx->system_ets = beam_ets_table_create("beam_system_ets", &ctx->system_alloc);
    if (!ctx->system_ets) {
        beam_atom_table_destroy(ctx->atom_table);
        beam_allocator_destroy(&ctx->system_alloc);
        return NULL;
    }

    /* Spawn Root Init Process (PID 1) */
    ctx->init_process = beam_process_create(1, 512, &ctx->system_alloc);
    if (!ctx->init_process) {
        beam_ets_table_destroy(ctx->system_ets);
        beam_atom_table_destroy(ctx->atom_table);
        beam_allocator_destroy(&ctx->system_alloc);
        return NULL;
    }

    return ctx;
}

void beam_context_destroy(beam_context_t* ctx) {
    if (!ctx) return;
    beam_allocator_i sys_alloc = ctx->system_alloc;

    if (ctx->init_process) beam_process_destroy(ctx->init_process);
    if (ctx->system_ets) beam_ets_table_destroy(ctx->system_ets);
    if (ctx->atom_table) beam_atom_table_destroy(ctx->atom_table);

    sys_alloc.free(sys_alloc.ctx, ctx);
    beam_allocator_destroy(&sys_alloc);
}

int main(void) {
    printf("=====================================================\n");
    printf(" BEAM Modular Monolith VM Starting Boot Sequence...\n");
    printf("=====================================================\n");

    beam_context_t* vm = beam_context_init();
    if (!vm) {
        fprintf(stderr, "FATAL: Failed to initialize BEAM VM Context!\n");
        return 1;
    }

    printf("  [CORE] Memory Allocator initialized.\n");
    printf("  [CORE] Atom Table initialized.\n");
    printf("  [CORE] System ETS Table initialized.\n");
    printf("  [CORE] Init Process PID 1 spawned successfully.\n");

    beam_context_destroy(vm);

    printf("[SUCCESS] BEAM VM Context shut down cleanly.\n");
    return 0;
}
