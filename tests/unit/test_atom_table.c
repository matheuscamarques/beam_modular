#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "beam_core.h"
#include "beam_global.h"
#include "mock_memory.h"

void test_atom_table_isolated(void) {
    printf("[UNIT TEST] Testing Atom Table in isolation...\n");

    mock_memory_stats_t stats = {0};
    beam_allocator_i alloc = mock_memory_create(&stats);

    beam_atom_table_t* table = beam_atom_table_create(&alloc, 16);
    assert(table != NULL);
    assert(beam_atom_table_count(table) == 0);

    uint32_t idx_ok, idx_err;
    beam_result_t res = beam_atom_put(table, "ok", 2, &idx_ok);
    assert(res == BEAM_OK);
    assert(idx_ok == 0);

    Eterm term_ok = make_atom_eterm(idx_ok);
    assert(eterm_to_atom_index(term_ok) == idx_ok);
    (void)term_ok;

    res = beam_atom_put(table, "error", 5, &idx_err);
    assert(res == BEAM_OK);
    assert(idx_err == 1);

    size_t len = 0;
    const char* name = beam_atom_get_name(table, idx_err, &len);
    assert(name != NULL && len == 5);
    assert(strcmp(name, "error") == 0);
    (void)name;
    (void)res;

    beam_atom_table_destroy(table);

    assert(stats.alloc_count > 0);
    assert(stats.free_count == stats.alloc_count);
    printf("  [MEMORY] Clean memory stats! Allocs: %zu, Frees: %zu\n", stats.alloc_count, stats.free_count);
    printf("[PASSED] test_atom_table_isolated\n");
}

int main(void) {
    test_atom_table_isolated();
    return 0;
}
