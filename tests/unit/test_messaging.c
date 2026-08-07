#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>

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

    beam_atom_table_t* atoms = beam_atom_table_create(&alloc, 16);
    assert(atoms != NULL);

    /* Enqueue messages */
    Eterm msg1 = ETERM_NIL;
    Eterm msg2 = beam_atom_intern(atoms, "msg_two");
    Eterm msg3 = beam_atom_intern(atoms, "msg_three");

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
    beam_atom_table_destroy(atoms);

    assert(stats.alloc_count > 0);
    assert(stats.free_count == stats.alloc_count);
    printf("  [MEMORY] Clean memory stats! Allocs: %zu, Frees: %zu\n", stats.alloc_count, stats.free_count);
    printf("[PASSED] test_mailbox_queue\n");
}

void test_process_to_process_messaging(void) {
    printf("[UNIT TEST] Testing Process-to-Process Message Delivery...\n");

    mock_memory_stats_t stats = {0};
    beam_allocator_i alloc = mock_memory_create(&stats);

    beam_atom_table_t* atoms = beam_atom_table_create(&alloc, 16);
    assert(atoms != NULL);

    beam_process_t* proc_sender   = beam_process_create(1001, 128, &alloc);
    beam_process_t* proc_receiver = beam_process_create(1002, 128, &alloc);

    assert(proc_sender && proc_receiver);

    Eterm tuple_msg_elems[2] = { beam_atom_intern(atoms, "tuple_ok"), make_small_int(500) };
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
    beam_atom_table_destroy(atoms);

    assert(stats.alloc_count > 0);
    assert(stats.free_count == stats.alloc_count);
    printf("  [RESULT] Process 1001 sent message tuple to Process 1002 successfully!\n");
    printf("  [PASSED] test_process_to_process_messaging\n");
}

typedef struct {
    beam_mailbox_t* mbox;
    int count;
} concurrent_sender_arg_t;

static void* concurrent_sender_thread(void* arg) {
    concurrent_sender_arg_t* sarg = (concurrent_sender_arg_t*)arg;
    for (int i = 0; i < sarg->count; i++) {
        beam_result_t res = beam_mailbox_enqueue(sarg->mbox, make_small_int(i));
        assert(res == BEAM_OK);
        (void)res;
    }
    return NULL;
}

void test_lock_free_concurrent_mailbox(void) {
    printf("[UNIT TEST] Testing Lock-Free Atomic Mailbox (4 Threads Enqueuing 1000 Msgs)...\n");

    mock_memory_stats_t stats = {0};
    beam_allocator_i alloc = mock_memory_create(&stats);

    beam_mailbox_t* mbox = beam_mailbox_create(&alloc);
    assert(mbox != NULL);

    pthread_t threads[4];
    concurrent_sender_arg_t args[4];

    for (int i = 0; i < 4; i++) {
        args[i].mbox = mbox;
        args[i].count = 250;
        int p_res = pthread_create(&threads[i], NULL, concurrent_sender_thread, &args[i]);
        assert(p_res == 0);
        (void)p_res;
    }

    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }

    assert(beam_mailbox_count(mbox) == 1000);

    /* Dequeue all 1000 messages */
    int count = 0;
    Eterm msg;
    while (beam_mailbox_dequeue(mbox, &msg) == BEAM_OK) {
        count++;
    }
    assert(count == 1000);
    assert(beam_mailbox_count(mbox) == 0);

    beam_mailbox_destroy(mbox);

    assert(stats.alloc_count > 0);
    assert(stats.free_count == stats.alloc_count);
    printf("  [RESULT] 4 Concurrent threads enqueued 1000 messages to lock-free mailbox without data loss!\n");
    printf("  [PASSED] test_lock_free_concurrent_mailbox\n");
}

int main(void) {
    printf("=========================================\n");
    printf(" RUNNING ISOLATED MODULE TEST: MESSAGING \n");
    printf("=========================================\n");
    test_mailbox_queue();
    test_process_to_process_messaging();
    test_lock_free_concurrent_mailbox();
    return 0;
}