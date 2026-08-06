#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "beam_core.h"
#include "beam_global.h"
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

int main(void) {
    printf("=========================================\n");
    printf(" RUNNING ISOLATED MODULE TEST: MESSAGING \n");
    printf("=========================================\n");
    test_mailbox_queue();
    return 0;
}
