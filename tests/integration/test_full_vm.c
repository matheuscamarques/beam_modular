#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "beam_core.h"
#include "beam_scheduler.h"
#include "mock_memory.h"

void test_full_vm_lifecycle(void) {
    printf("[INTEGRATION TEST] Testing Full BEAM VM Lifecycle & Orchestrator...\n");

    mock_memory_stats_t stats = {0};
    beam_allocator_i alloc = mock_memory_create(&stats);

    beam_context_t* vm = beam_vm_create(&alloc);
    assert(vm != NULL);

    beam_code_server_t* cs = beam_vm_get_code_server(vm);
    assert(cs != NULL);
    assert(beam_code_server_module_count(cs) == 0);
    (void)cs;

    beam_vm_destroy(vm);

    assert(stats.alloc_count > 0);
    assert(stats.free_count == stats.alloc_count);
    printf("  [MEMORY] Clean memory stats! Allocs: %zu, Frees: %zu\n", stats.alloc_count, stats.free_count);
    printf("[PASSED] test_full_vm_lifecycle\n");
}

int main(void) {
    printf("=========================================\n");
    printf(" RUNNING INTEGRATION TEST: FULL BEAM VM  \n");
    printf("=========================================\n");
    test_full_vm_lifecycle();
    return 0;
}
