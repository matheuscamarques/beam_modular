#include "beam_bif_internal.h"

Eterm bif_self_0(beam_process_t* proc, const Eterm* args, int arity) {
    (void)args;
    (void)arity;
    if (!proc) return ETERM_INVALID;
    uint32_t pid = beam_process_get_pid(proc);
    return (Eterm)((pid << 4) | TAG_IMMED1_PID);
}

Eterm bif_add_2(beam_process_t* proc, const Eterm* args, int arity) {
    (void)proc;
    if (arity < 2 || !args) return ETERM_INVALID;
    intptr_t a = eterm_to_small_int(args[0]);
    intptr_t b = eterm_to_small_int(args[1]);
    return make_small_int(a + b);
}

Eterm bif_sub_2(beam_process_t* proc, const Eterm* args, int arity) {
    (void)proc;
    if (arity < 2 || !args) return ETERM_INVALID;
    intptr_t a = eterm_to_small_int(args[0]);
    intptr_t b = eterm_to_small_int(args[1]);
    return make_small_int(a - b);
}

static const beam_bif_entry_t bif_table[] = {
    { .name = "erlang:self/0", .arity = 0, .handler = bif_self_0 },
    { .name = "erlang:+/2",    .arity = 2, .handler = bif_add_2  },
    { .name = "erlang:-/2",    .arity = 2, .handler = bif_sub_2  }
};

static const size_t num_bifs = sizeof(bif_table) / sizeof(bif_table[0]);

beam_result_t beam_bif_dispatch(size_t bif_index, beam_process_t* proc, const Eterm* args, int arity, Eterm* out_result) {
    if (bif_index >= num_bifs || !out_result) {
        return BEAM_ERR_INVALID_ARG;
    }

    const beam_bif_entry_t* bif = &bif_table[bif_index];
    if (bif->arity != arity) {
        return BEAM_ERR_BADARG;
    }

    *out_result = bif->handler(proc, args, arity);
    return BEAM_OK;
}
