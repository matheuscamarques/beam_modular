#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "beam_core.h"
#include "beam_io.h"
#include "mock_memory.h"

void test_io_poller(void) {
    printf("[UNIT TEST] Testing I/O Driver Poller...\n");

    mock_memory_stats_t stats = {0};
    beam_allocator_i alloc = mock_memory_create(&stats);

    beam_io_poller_t* poller = beam_io_poller_create(&alloc);
    assert(poller != NULL);

    beam_result_t res;
    res = beam_io_poller_register(poller, 3, BEAM_IO_READABLE);
    assert(res == BEAM_OK);

    res = beam_io_poller_register(poller, 4, BEAM_IO_WRITABLE);
    assert(res == BEAM_OK);

    int ready = 0;
    res = beam_io_poller_poll(poller, 10, &ready);
    assert(res == BEAM_OK);
    assert(ready == 2);
    (void)res;

    beam_io_poller_destroy(poller);

    assert(stats.alloc_count > 0);
    assert(stats.free_count == stats.alloc_count);
    printf("  [MEMORY] Clean memory stats! Allocs: %zu, Frees: %zu\n", stats.alloc_count, stats.free_count);
    printf("[PASSED] test_io_poller\n");
}

int main(void) {
    printf("=========================================\n");
    printf(" RUNNING ISOLATED MODULE TEST: I/O      \n");
    printf("=========================================\n");
    test_io_poller();
    return 0;
}
