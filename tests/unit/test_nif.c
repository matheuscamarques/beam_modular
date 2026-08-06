#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "beam_core.h"
#include "beam_scheduler.h"
#include "beam_io.h"
#include "mock_memory.h"

/* Simulated C NIF Function: nif_add_tuple_impl(env, argc, argv) */
static ERL_NIF_TERM nif_add_impl(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]) {
    if (argc != 2) return ETERM_INVALID;

    int a = 0, b = 0;
    if (!enif_get_int(env, argv[0], &a) || !enif_get_int(env, argv[1], &b)) {
        return ETERM_INVALID;
    }

    ERL_NIF_TERM sum = enif_make_int(env, a + b);
    ERL_NIF_TERM elems[2] = { make_small_int(10), sum };
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

    ERL_NIF_TERM args[2] = { enif_make_int(env, 300), enif_make_int(env, 400) };
    ERL_NIF_TERM res_tuple = nif_add_impl(env, 2, args);

    assert(beam_is_tuple(res_tuple) == true);
    assert(beam_tuple_arity(res_tuple) == 2);
    (void)res_tuple;

    int sum_val = 0;
    assert(enif_get_int(env, beam_tuple_element(res_tuple, 1), &sum_val) == true);
    assert(sum_val == 700);
    (void)sum_val;

    enif_free_env(env);
    beam_process_destroy(proc);

    assert(stats.alloc_count > 0);
    assert(stats.free_count == stats.alloc_count);
    printf("  [RESULT] NIF executed successfully! 300 + 400 = 700 returned in Tuple!\n");
    printf("  [PASSED] test_nif_extension\n");
}

int main(void) {
    printf("=========================================\n");
    printf(" RUNNING ISOLATED MODULE TEST: NIF C-EXT \n");
    printf("=========================================\n");
    test_nif_extension();
    return 0;
}
