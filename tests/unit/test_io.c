#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "beam_core.h"
#include "beam_io.h"
#include "beam_scheduler.h"
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

#include "beam_messaging.h"

void test_native_tcp_socket_dispatch(void) {
    printf("[UNIT TEST] Testing Native TCP Socket Server & Process Mailbox Dispatch (gen_tcp)...\n");

    mock_memory_stats_t stats = {0};
    beam_allocator_i alloc = mock_memory_create(&stats);

    beam_process_t* proc = beam_process_create(1201, 128, &alloc);
    assert(proc != NULL);

    int sv[2] = {-1, -1};
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0);

    const char* msg = "HELLO_BEAM_C23";
    ssize_t w_res = write(sv[1], msg, 14);
    assert(w_res == 14);
    (void)w_res;

    beam_result_t res = beam_socket_dispatch_mailbox(proc, sv[0], &alloc);
    assert(res == BEAM_OK);

    Eterm rec_msg = 0;
    res = beam_process_receive_message(proc, &rec_msg);
    assert(res == BEAM_OK);
    (void)res;
    assert(eterm_to_small_int(rec_msg) == 14);

    close(sv[0]);
    close(sv[1]);
    beam_process_destroy(proc);

    assert(stats.alloc_count > 0);
    assert(stats.free_count == stats.alloc_count);
    printf("  [RESULT] Socket payload dispatched directly into Process Mailbox as message term!\n");
    printf("[PASSED] test_native_tcp_socket_dispatch\n");
}

int main(void) {
    printf("=========================================\n");
    printf(" RUNNING ISOLATED MODULE TEST: I/O      \n");
    printf("=========================================\n");
    test_io_poller();
    test_native_tcp_socket_dispatch();
    return 0;
}
