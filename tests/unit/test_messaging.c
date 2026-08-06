#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "beam_core.h"
#include "beam_global.h"
#include "beam_scheduler.h"
#include "beam_messaging.h"
#include "mock_memory.h"

void test_mailbox_queue(void) {
    printf("[UNIT TEST] Testing Mailbox Queue & Message Passing...\n");

    mock_memory_stats_t stats = {0};
    beam_allocator_i alloc = mock_memory_create(&stats);

    beam_mailbox_t* mbox = beam_mailbox_create(&alloc);
    assert(mbox != NULL);
    assert(beam_mailbox_count(mbox) == 0);

    /* Enqueue messages */
    Eterm msg1 = ETERM_NIL;
    Eterm msg2 = make_atom_eterm(42);
    Eterm msg3 = make_atom_eterm(100);

    beam_result_t res;
    res = beam_mailbox_enqueue(mbox, msg1);
    assert(res == BEAM_OK);
    res = beam_mailbox_enqueue(mbox, msg2);
    assert(res == BEAM_OK);
    res = beam_mailbox_enqueue(mbox, msg3);
    assert(res == BEAM_OK);
    assert(beam_mailbox_count(mbox) == 3);

    /* Dequeue and verify FIFO ordering */
    Eterm out = 0;
    res = beam_mailbox_dequeue(mbox, &out);
    assert(res == BEAM_OK);
    assert(out == msg1);

    res = beam_mailbox_dequeue(mbox, &out);
    assert(res == BEAM_OK);
    assert(out == msg2);

    res = beam_mailbox_dequeue(mbox, &out);
    assert(res == BEAM_OK);
    assert(out == msg3);

    assert(beam_mailbox_count(mbox) == 0);
    res = beam_mailbox_dequeue(mbox, &out);
    assert(res == BEAM_ERR_NOT_FOUND);
    (void)res;

    beam_mailbox_destroy(mbox);

    assert(stats.alloc_count > 0);
    assert(stats.free_count == stats.alloc_count);
    printf("  [MEMORY] Clean memory stats! Allocs: %zu, Frees: %zu\n", stats.alloc_count, stats.free_count);
    printf("[PASSED] test_mailbox_queue\n");
}

void test_process_to_process_messaging(void) {
    printf("[UNIT TEST] Testing Process-to-Process Message Delivery...\n");

    mock_memory_stats_t stats = {0};
    beam_allocator_i alloc = mock_memory_create(&stats);

    beam_process_t* proc_sender   = beam_process_create(1001, 128, &alloc);
    beam_process_t* proc_receiver = beam_process_create(1002, 128, &alloc);

    assert(proc_sender && proc_receiver);

    Eterm tuple_msg_elems[2] = { make_atom_eterm(1), make_small_int(500) };
    Eterm tuple_msg = beam_make_tuple(proc_sender, 2, tuple_msg_elems);

    /* Send message from sender to receiver process */
    beam_result_t res = beam_message_send_to_process(proc_receiver, tuple_msg, &alloc);
    assert(res == BEAM_OK);

    /* Receiver receives message from its mailbox */
    Eterm received_msg = 0;
    res = beam_process_receive_message(proc_receiver, &received_msg);
    assert(res == BEAM_OK);
    (void)res;

    assert(beam_is_tuple(received_msg) == true);
    assert(eterm_to_small_int(beam_tuple_element(received_msg, 1)) == 500);

    beam_process_destroy(proc_sender);
    beam_process_destroy(proc_receiver);

    assert(stats.alloc_count > 0);
    assert(stats.free_count == stats.alloc_count);
    printf("  [RESULT] Process 1001 sent message tuple to Process 1002 successfully!\n");
    printf("  [PASSED] test_process_to_process_messaging\n");
}

int main(void) {
    printf("=========================================\n");
    printf(" RUNNING ISOLATED MODULE TEST: MESSAGING \n");
    printf("=========================================\n");
    test_mailbox_queue();
    test_process_to_process_messaging();
    return 0;
}
