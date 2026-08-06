#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "beam_core.h"
#include "beam_scheduler.h"
#include "mock_memory.h"

void test_tuple_terms(void) {
    printf("[UNIT TEST] Testing Tuple Compound Terms...\n");

    mock_memory_stats_t stats = {0};
    beam_allocator_i alloc = mock_memory_create(&stats);

    beam_process_t* proc = beam_process_create(888, 128, &alloc);
    assert(proc != NULL);

    Eterm elems[3] = { make_small_int(1), make_small_int(2), make_small_int(3) };
    Eterm tup = beam_make_tuple(proc, 3, elems);

    assert(tup != ETERM_INVALID);
    assert(beam_is_tuple(tup) == true);
    assert(beam_tuple_arity(tup) == 3);

    assert(eterm_to_small_int(beam_tuple_element(tup, 0)) == 1);
    assert(eterm_to_small_int(beam_tuple_element(tup, 1)) == 2);
    assert(eterm_to_small_int(beam_tuple_element(tup, 2)) == 3);
    (void)tup;

    beam_process_destroy(proc);

    assert(stats.alloc_count > 0);
    assert(stats.free_count == stats.alloc_count);
    printf("  [PASSED] Tuple {1, 2, 3} verified!\n");
}

void test_list_terms(void) {
    printf("[UNIT TEST] Testing List (Cons Cells) Compound Terms...\n");

    mock_memory_stats_t stats = {0};
    beam_allocator_i alloc = mock_memory_create(&stats);

    beam_process_t* proc = beam_process_create(999, 128, &alloc);
    assert(proc != NULL);

    /* Construct list [10, 20] -> [10 | [20 | NIL]] */
    Eterm list2 = beam_make_list(proc, make_small_int(20), ETERM_NIL);
    Eterm list1 = beam_make_list(proc, make_small_int(10), list2);

    assert(beam_is_list(list1) == true);
    assert(eterm_to_small_int(beam_list_head(list1)) == 10);

    Eterm tail1 = beam_list_tail(list1);
    assert(beam_is_list(tail1) == true);
    assert(eterm_to_small_int(beam_list_head(tail1)) == 20);
    assert(beam_is_nil(beam_list_tail(tail1)) == true);
    (void)tail1;

    beam_process_destroy(proc);

    assert(stats.alloc_count > 0);
    assert(stats.free_count == stats.alloc_count);
    printf("  [PASSED] List [10, 20] verified!\n");
}

int main(void) {
    printf("=========================================\n");
    printf(" RUNNING ISOLATED MODULE TEST: TERMS    \n");
    printf("=========================================\n");
    test_tuple_terms();
    test_list_terms();
    return 0;
}
