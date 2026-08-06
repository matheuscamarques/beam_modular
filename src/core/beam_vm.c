#include "beam_main_internal.h"
#include <string.h>

beam_context_t* beam_vm_create(const beam_allocator_i* alloc) {
    if (!alloc || !alloc->alloc || !alloc->free) return NULL;

    beam_context_t* ctx = (beam_context_t*)alloc->alloc(alloc->ctx, sizeof(beam_context_t));
    if (!ctx) return NULL;

    memset(ctx, 0, sizeof(beam_context_t));
    ctx->alloc = *alloc;

    ctx->code_server = beam_code_server_create(alloc);
    ctx->atom_table = beam_atom_table_create(alloc, 1024);
    ctx->ets_table = beam_ets_table_create("system_ets", alloc);
    ctx->poller = beam_io_poller_create(alloc);
    ctx->run_queue = beam_run_queue_create(alloc);
    ctx->scheduler = beam_scheduler_create(1, ctx->run_queue, alloc);

    if (!ctx->code_server || !ctx->atom_table || !ctx->ets_table ||
        !ctx->poller || !ctx->run_queue || !ctx->scheduler) {
        beam_vm_destroy(ctx);
        return NULL;
    }

    return ctx;
}

void beam_vm_destroy(beam_context_t* ctx) {
    if (!ctx) return;
    beam_allocator_i alloc = ctx->alloc;

    if (ctx->scheduler) beam_scheduler_destroy(ctx->scheduler);
    if (ctx->run_queue) beam_run_queue_destroy(ctx->run_queue);
    if (ctx->poller) beam_io_poller_destroy(ctx->poller);
    if (ctx->ets_table) beam_ets_table_destroy(ctx->ets_table);
    if (ctx->atom_table) beam_atom_table_destroy(ctx->atom_table);
    if (ctx->code_server) beam_code_server_destroy(ctx->code_server);

    alloc.free(alloc.ctx, ctx);
}

beam_code_server_t* beam_vm_get_code_server(const beam_context_t* ctx) {
    return ctx ? ctx->code_server : NULL;
}
