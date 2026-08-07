#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "beam_core.h"
#include "beam_io.h"
#include "mock_memory.h"

#include <unistd.h>
#include <sys/socket.h>

void test_io_poller(void) {
    printf("[UNIT TEST] Testing I/O Driver Poller (Real Linux Epoll)...\n");

    mock_memory_stats_t stats = {0};
    beam_allocator_i alloc = mock_memory_create(&stats);

    beam_io_poller_t* poller = beam_io_poller_create(&alloc);
    assert(poller != NULL);

    int sv[2] = {-1, -1};
    int sp_res = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
    assert(sp_res == 0);
    (void)sp_res;

    beam_result_t res = beam_io_poller_register(poller, sv[0], BEAM_IO_READABLE);
    assert(res == BEAM_OK);

    /* Write byte to sv[1] so sv[0] becomes readable */
    char buf = 'A';
    ssize_t w_res = write(sv[1], &buf, 1);
    assert(w_res == 1);
    (void)w_res;

    int ready = 0;
    res = beam_io_poller_poll(poller, 10, &ready);
    assert(res == BEAM_OK);
    assert(ready == 1);
    (void)res;

    close(sv[0]);
    close(sv[1]);
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
