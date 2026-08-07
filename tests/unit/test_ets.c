#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "beam_core.h"
#include "beam_global.h"
#include "mock_memory.h"

void test_ets_table(void) {
    printf("[UNIT TEST] Testing ETS Database Operations...\n");

    mock_memory_stats_t stats = {0};
    beam_allocator_i alloc = mock_memory_create(&stats);

    beam_atom_table_t* atoms = beam_atom_table_create(&alloc, 16);
    assert(atoms != NULL);

    beam_ets_table_t* ets = beam_ets_table_create("user_sessions", &alloc);
    assert(ets != NULL);
    assert(beam_ets_count(ets) == 0);

    Eterm key1 = beam_atom_intern(atoms, "user_10");
    Eterm val1 = beam_atom_intern(atoms, "active_999");
    assert(key1 != ETERM_INVALID && val1 != ETERM_INVALID);

    Eterm key2 = beam_atom_intern(atoms, "user_20");
    Eterm val2 = beam_atom_intern(atoms, "active_888");
    assert(key2 != ETERM_INVALID && val2 != ETERM_INVALID);

    beam_result_t res;
    res = beam_ets_insert(ets, key1, val1);
    assert(res == BEAM_OK);
    res = beam_ets_insert(ets, key2, val2);
    assert(res == BEAM_OK);
    assert(beam_ets_count(ets) == 2);

    Eterm out_val = 0;
    res = beam_ets_lookup(ets, key1, &out_val);
    assert(res == BEAM_OK);
    assert(out_val == val1);

    res = beam_ets_lookup(ets, key2, &out_val);
    assert(res == BEAM_OK);
    assert(out_val == val2);

    /* Delete key1 */
    res = beam_ets_delete(ets, key1);
    assert(res == BEAM_OK);
    assert(beam_ets_count(ets) == 1);

    res = beam_ets_lookup(ets, key1, &out_val);
    assert(res == BEAM_ERR_NOT_FOUND);

    /* Test ETS update counter */
    Eterm counter_key = beam_atom_intern(atoms, "request_count");
    res = beam_ets_insert(ets, counter_key, make_small_int(100));
    assert(res == BEAM_OK);

    Eterm new_counter = 0;
    res = beam_ets_update_counter(ets, counter_key, 5, &new_counter);
    assert(res == BEAM_OK);
    assert(eterm_to_small_int(new_counter) == 105);

    /* Test BAG table type (allows duplicate keys with different values, ignores identical pairs) */
    beam_ets_table_t* ets_bag = beam_ets_table_create_typed("events_bag", 2 /* BAG */, 0 /* PUBLIC */, &alloc);
    assert(ets_bag != NULL);

    Eterm bag_key = beam_atom_intern(atoms, "event_log");
    Eterm event1 = beam_atom_intern(atoms, "login");
    Eterm event2 = beam_atom_intern(atoms, "logout");

    res = beam_ets_insert(ets_bag, bag_key, event1);
    assert(res == BEAM_OK);
    res = beam_ets_insert(ets_bag, bag_key, event2);
    assert(res == BEAM_OK);
    assert(beam_ets_count(ets_bag) == 2);

    /* Duplicate pair insert attempt -> ignored */
    res = beam_ets_insert(ets_bag, bag_key, event1);
    assert(res == BEAM_OK);
    assert(beam_ets_count(ets_bag) == 2);

    (void)res;

    beam_ets_table_destroy(ets_bag);
    beam_ets_table_destroy(ets);
    beam_atom_table_destroy(atoms);

    assert(stats.alloc_count > 0);
    assert(stats.free_count == stats.alloc_count);
    printf("  [MEMORY] Clean memory stats! Allocs: %zu, Frees: %zu\n", stats.alloc_count, stats.free_count);
    printf("[PASSED] test_ets_table\n");
}

int main(void) {
    printf("=========================================\n");
    printf(" RUNNING ISOLATED MODULE TEST: ETS DB    \n");
    printf("=========================================\n");
    test_ets_table();
    return 0;
}
