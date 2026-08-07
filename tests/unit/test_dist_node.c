#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "beam_core.h"
#include "beam_global.h"
#include "mock_memory.h"

void test_distributed_node_table(void) {
    printf("[UNIT TEST] Testing Distributed Erlang Node Table...\n");

    mock_memory_stats_t stats = {0};
    beam_allocator_i alloc = mock_memory_create(&stats);

    beam_node_table_t* nt = beam_node_table_create(&alloc);
    assert(nt != NULL);
    assert(beam_node_table_count(nt) == 0);

    beam_result_t res;
    res = beam_node_table_connect(nt, "node1@127.0.0.1");
    assert(res == BEAM_OK);

    res = beam_node_table_connect(nt, "node2@cluster.local");
    assert(res == BEAM_OK);
    assert(beam_node_table_count(nt) == 2);
    (void)res;

    assert(beam_node_table_is_connected(nt, "node1@127.0.0.1") == true);
    assert(beam_node_table_is_connected(nt, "node2@cluster.local") == true);
    assert(beam_node_table_is_connected(nt, "unknown@node") == false);

    /* Test EPMD Registration & Custom Port Connection */
    assert(beam_node_epmd_register("node3@cluster.local", 4370) == BEAM_OK);
    assert(beam_node_table_connect_port(nt, "node3@cluster.local", 4370) == BEAM_OK);
    assert(beam_node_table_count(nt) == 3);
    assert(beam_node_table_is_connected(nt, "node3@cluster.local") == true);

    beam_node_table_destroy(nt);

    assert(stats.alloc_count > 0);
    assert(stats.free_count == stats.alloc_count);
    printf("  [RESULT] Distributed nodes & EPMD handshake connected & verified cleanly!\n");
    printf("  [PASSED] test_distributed_node_table\n");
}

int main(void) {
    printf("=========================================\n");
    printf(" RUNNING ISOLATED MODULE TEST: DIST NODE \n");
    printf("=========================================\n");
    test_distributed_node_table();
    return 0;
}
