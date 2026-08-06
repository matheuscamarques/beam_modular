#include "beam_bif_internal.h"

Eterm bif_self_0(beam_process_t* proc, const Eterm* args, int arity) {
    (void)arity;
    (void)args;
    if (!proc) return 0;
    return (Eterm)(beam_process_get_pid(proc) << 4 | TAG_IMMED1_PID);
}

Eterm bif_add_2(beam_process_t* proc, const Eterm* args, int arity) {
    (void)proc;
    if (arity < 2 || !args) return 0;
    intptr_t v1 = eterm_to_small_int(args[0]);
    intptr_t v2 = eterm_to_small_int(args[1]);
    return make_small_int(v1 + v2);
}

Eterm bif_sub_2(beam_process_t* proc, const Eterm* args, int arity) {
    (void)proc;
    if (arity < 2 || !args) return 0;
    intptr_t v1 = eterm_to_small_int(args[0]);
    intptr_t v2 = eterm_to_small_int(args[1]);
    return make_small_int(v1 - v2);
}

Eterm bif_is_atom_1(beam_process_t* proc, const Eterm* args, int arity) {
    (void)proc;
    if (arity < 1 || !args) return 0;
    return beam_is_atom(args[0]) ? (1 << 4 | TAG_IMMED1_ATOM) : (2 << 4 | TAG_IMMED1_ATOM);
}

Eterm bif_is_tuple_1(beam_process_t* proc, const Eterm* args, int arity) {
    (void)proc;
    if (arity < 1 || !args) return 0;
    return beam_is_tuple(args[0]) ? (1 << 4 | TAG_IMMED1_ATOM) : (2 << 4 | TAG_IMMED1_ATOM);
}

Eterm bif_length_1(beam_process_t* proc, const Eterm* args, int arity) {
    (void)proc;
    if (arity < 1 || !args) return 0;
    Eterm list = args[0];
    size_t len = 0;
    while (beam_is_list(list)) {
        len++;
        list = beam_list_tail(list);
    }
    return make_small_int(len);
}

beam_result_t beam_bif_dispatch(size_t bif_index, beam_process_t* proc, const Eterm* args, int arity, Eterm* out_result) {
    if (!proc || !out_result) return BEAM_ERR_INVALID_ARG;
    if (bif_index == 0) {
        *out_result = bif_self_0(proc, args, arity);
        return BEAM_OK;
    } else if (bif_index == 1) {
        *out_result = bif_add_2(proc, args, arity);
        return BEAM_OK;
    } else if (bif_index == 2) {
        *out_result = bif_sub_2(proc, args, arity);
        return BEAM_OK;
    } else if (bif_index == 3) {
        *out_result = bif_is_atom_1(proc, args, arity);
        return BEAM_OK;
    } else if (bif_index == 4) {
        *out_result = bif_is_tuple_1(proc, args, arity);
        return BEAM_OK;
    } else if (bif_index == 5) {
        *out_result = bif_length_1(proc, args, arity);
        return BEAM_OK;
    }
    return BEAM_ERR_NOT_FOUND;
}
