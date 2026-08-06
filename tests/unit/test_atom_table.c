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

    Eterm atom_ok = beam_atom_intern(table, "ok");
    assert(atom_ok != 0);
    assert(beam_atom_table_count(table) == 1);

    Eterm atom_err = beam_atom_intern(table, "error");
    assert(atom_err != 0);
    assert(beam_atom_table_count(table) == 2);

    Eterm atom_ok_repeat = beam_atom_intern(table, "ok");
    assert(atom_ok_repeat == atom_ok);
    assert(beam_atom_table_count(table) == 2);
    (void)atom_ok_repeat;

    /* Lookup atom name */
    const char* name_ok = beam_atom_lookup(table, atom_ok);
    assert(name_ok != NULL && strcmp(name_ok, "ok") == 0);

    const char* name_err = beam_atom_lookup(table, atom_err);
    assert(name_err != NULL && strcmp(name_err, "error") == 0);
    (void)name_ok;
    (void)name_err;

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
