#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "beam_atom.h"
#include "mocks/mock_allocator.h"

void test_atom_table_lifecycle(void) {
    printf("[TEST] Atom Table Lifecycle & Operations...\n");

    mock_allocator_stats_t stats = {0};
    beam_allocator_i alloc = mock_allocator_create(&stats);

    beam_atom_table_t* table = beam_atom_table_create(&alloc, 16);
    assert(table != NULL);
    assert(beam_atom_table_count(table) == 0);

    /* Insert atoms */
    uint32_t idx_ok, idx_error, idx_undefined, idx_ok2;
    beam_atom_result_t res;

    res = beam_atom_put(table, "ok", 2, &idx_ok);
    assert(res == BEAM_ATOM_OK);
    assert(idx_ok == 0);

    res = beam_atom_put(table, "error", 5, &idx_error);
    assert(res == BEAM_ATOM_OK);
    assert(idx_error == 1);

    res = beam_atom_put(table, "undefined", 9, &idx_undefined);
    assert(res == BEAM_ATOM_OK);
    assert(idx_undefined == 2);

    assert(beam_atom_table_count(table) == 3);

    /* Test duplicate insertion returns same index */
    res = beam_atom_put(table, "ok", 2, &idx_ok2);
    assert(res == BEAM_ATOM_OK);
    assert(idx_ok2 == idx_ok);
    assert(beam_atom_table_count(table) == 3);

    /* Test lookup by index */
    size_t len = 0;
    const char* name = beam_atom_get_name(table, idx_error, &len);
    assert(name != NULL);
    assert(len == 5);
    assert(strcmp(name, "error") == 0);

    /* Test find without insertion */
    uint32_t found_idx;
    res = beam_atom_find(table, "undefined", 9, &found_idx);
    assert(res == BEAM_ATOM_OK);
    assert(found_idx == idx_undefined);

    res = beam_atom_find(table, "nonexistent", 11, &found_idx);
    assert(res == BEAM_ATOM_ERR_NOT_FOUND);

    /* Destroy table and verify zero memory leaks */
    beam_atom_table_destroy(table);

    printf("  [ALLOC STATS] Allocs: %zu | Frees: %zu\n", stats.alloc_count, stats.free_count);
    assert(stats.alloc_count > 0);
    assert(stats.free_count == stats.alloc_count);

    printf("[PASSED] test_atom_table_lifecycle passed cleanly!\n");
}

int main(void) {
    printf("=========================================\n");
    printf(" RUNNING ISOLATED MODULE TEST: ATOM TABLE\n");
    printf("=========================================\n");
    test_atom_table_lifecycle();
    return 0;
}
