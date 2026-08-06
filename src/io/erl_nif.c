#include "erl_nif_internal.h"
#include <string.h>

ErlNifEnv* enif_alloc_env(const beam_allocator_i* alloc, beam_process_t* proc) {
    if (!alloc || !alloc->alloc || !alloc->free) return NULL;

    ErlNifEnv* env = (ErlNifEnv*)alloc->alloc(alloc->ctx, sizeof(ErlNifEnv));
    if (!env) return NULL;

    memset(env, 0, sizeof(ErlNifEnv));
    env->alloc = *alloc;
    env->proc = proc;
    return env;
}

void enif_free_env(ErlNifEnv* env) {
    if (!env) return;
    beam_allocator_i alloc = env->alloc;
    alloc.free(alloc.ctx, env);
}

ERL_NIF_TERM enif_make_int(ErlNifEnv* env, int val) {
    (void)env;
    return make_small_int(val);
}

bool enif_get_int(ErlNifEnv* env, ERL_NIF_TERM term, int* out_val) {
    (void)env;
    if (!beam_is_small_int(term) || !out_val) return false;
    *out_val = (int)eterm_to_small_int(term);
    return true;
}

ERL_NIF_TERM enif_make_tuple_from_array(ErlNifEnv* env, const ERL_NIF_TERM* elements, unsigned int cnt) {
    if (!env || !env->proc) return ETERM_INVALID;
    return beam_make_tuple(env->proc, (size_t)cnt, elements);
}
