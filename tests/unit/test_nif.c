#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "beam_core.h"
#include "beam_scheduler.h"
#include "beam_io.h"
#include "mock_memory.h"

/* Simulated C NIF Function: nif_add_tuple_impl(env, argc, argv) */
static Eterm nif_add_impl(ErlNifEnv* env, int argc, const Eterm argv[]) {
    if (argc != 2) return 0;

    int a = 0, b = 0;
    if (!enif_get_int(env, argv[0], &a) || !enif_get_int(env, argv[1], &b)) {
        return 0;
    }

    Eterm sum = enif_make_int(env, a + b);
    Eterm elems[2] = { make_small_int(10), sum };
    return enif_make_tuple_from_array(env, elems, 2);
}

void test_nif_extension(void) {
    printf("[UNIT TEST] Testing NIF C-Extension Binding & Environment...\n");

    mock_memory_stats_t stats = {0};
    beam_allocator_i alloc = mock_memory_create(&stats);

    beam_process_t* proc = beam_process_create(801, 256, &alloc);
    assert(proc != NULL);

    ErlNifEnv* env = enif_alloc_env(&alloc, proc);
    assert(env != NULL);

    Eterm args[2] = { enif_make_int(env, 300), enif_make_int(env, 400) };
    Eterm res_tuple = nif_add_impl(env, 2, args);

    assert(beam_is_tuple(res_tuple) == true);
    assert(beam_tuple_arity(res_tuple) == 2);
    (void)res_tuple;

    int sum_val = 0;
    assert(enif_get_int(env, beam_tuple_element(res_tuple, 1), &sum_val) == true);
    assert(sum_val == 700);
    (void)sum_val;

    /* Test dlopen/dlsym functionality on standard math library */
    void* handle = enif_dlopen("libm.so.6");
    if (!handle) handle = enif_dlopen("libm.so");
    if (handle) {
        void* cos_fn = enif_dlsym(handle, "cos");
        assert(cos_fn != NULL);
        (void)cos_fn;
        enif_dlclose(handle);
    }

    enif_free_env(env);
    beam_process_destroy(proc);

    assert(stats.alloc_count > 0);
    assert(stats.free_count == stats.alloc_count);
    printf("  [RESULT] NIF & dlopen executed successfully! 300 + 400 = 700 returned in Tuple!\n");
    printf("  [PASSED] test_nif_extension\n");
}

int main(void) {
    printf("=========================================\n");
    printf(" RUNNING ISOLATED MODULE TEST: NIF C-EXT \n");
    printf("=========================================\n");
    test_nif_extension();
    return 0;
}
