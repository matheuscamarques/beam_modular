#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "beam_core.h"
#include "beam_scheduler.h"
#include "beam_messaging.h"
#include "beam_emu_internal.h"
#include "mock_memory.h"

void test_opcode_execution(void) {
    printf("[UNIT TEST] Testing OpCode Interpreter Execution...\n");

    mock_memory_stats_t stats = {0};
    beam_allocator_i alloc = mock_memory_create(&stats);

    beam_process_t* proc = beam_process_create(201, 128, &alloc);
    assert(proc != NULL);

    beam_instruction_t code[] = {
        { .opcode = BEAM_OP_MOVE, .arg1 = 0, .arg2 = 0, .literal = make_small_int(10) },
        { .opcode = BEAM_OP_MOVE, .arg1 = 0, .arg2 = 1, .literal = make_small_int(20) },
        { .opcode = BEAM_OP_ADD,  .arg1 = 0, .arg2 = 1, .arg3 = 0 },
        { .opcode = BEAM_OP_HALT }
    };

    Eterm result = 0;
    beam_result_t res = beam_emu_execute_code(proc, code, sizeof(code)/sizeof(code[0]), &result);

    assert(res == BEAM_ERR_HALT);
    (void)res;
    assert(eterm_to_small_int(result) == 30);
    assert(beam_process_get_state(proc) == BEAM_PROC_STATE_EXITED);

    beam_process_destroy(proc);

    assert(stats.alloc_count > 0);
    assert(stats.free_count == stats.alloc_count);
    printf("  [RESULT] Executed 10 + 20 = %ld cleanly!\n", (long)eterm_to_small_int(result));
    printf("  [PASSED] test_opcode_execution\n");
}

void test_call_stack_execution(void) {
    printf("[UNIT TEST] Testing Stack Allocation & Function Call/Return...\n");

    mock_memory_stats_t stats = {0};
    beam_allocator_i alloc = mock_memory_create(&stats);

    beam_process_t* proc = beam_process_create(202, 128, &alloc);
    assert(proc != NULL);

    beam_instruction_t code[] = {
        { .opcode = BEAM_OP_ALLOCATE,   .arg1 = 1 },
        { .opcode = BEAM_OP_MOVE,       .arg1 = 0, .arg2 = 0, .literal = make_small_int(100) },
        { .opcode = BEAM_OP_CALL,       .arg1 = 5 },
        { .opcode = BEAM_OP_DEALLOCATE, .arg1 = 1 },
        { .opcode = BEAM_OP_HALT },
        { .opcode = BEAM_OP_MOVE,       .arg1 = 0, .arg2 = 1, .literal = make_small_int(50) },
        { .opcode = BEAM_OP_ADD,        .arg1 = 0, .arg2 = 1, .arg3 = 0 },
        { .opcode = BEAM_OP_RETURN }
    };

    Eterm result = 0;
    beam_result_t res = beam_emu_execute_code(proc, code, sizeof(code)/sizeof(code[0]), &result);

    assert(res == BEAM_ERR_HALT);
    (void)res;
    assert(eterm_to_small_int(result) == 150);

    beam_process_destroy(proc);

    assert(stats.alloc_count > 0);
    assert(stats.free_count == stats.alloc_count);
    printf("  [RESULT] Function Call 100 + 50 = %ld via CALL/RETURN!\n", (long)eterm_to_small_int(result));
    printf("  [PASSED] test_call_stack_execution\n");
}

void test_opcode_messaging(void) {
    printf("[UNIT TEST] Testing OpCode Messaging (OP_SEND and OP_RECEIVE)...\n");

    mock_memory_stats_t stats = {0};
    beam_allocator_i alloc = mock_memory_create(&stats);

    beam_process_t* proc_sender   = beam_process_create(501, 128, &alloc);
    beam_process_t* proc_receiver = beam_process_create(502, 128, &alloc);

    /* Sender Bytecode: MOVE 777 -> X[1], SEND X[1] to proc_receiver, HALT */
    beam_instruction_t sender_code[] = {
        { .opcode = BEAM_OP_MOVE, .arg1 = 0, .arg2 = 1, .literal = make_small_int(777) },
        { .opcode = BEAM_OP_SEND, .arg1 = 1, .target_proc = proc_receiver },
        { .opcode = BEAM_OP_HALT }
    };

    /* Receiver Bytecode: RECEIVE -> X[0], HALT */
    beam_instruction_t receiver_code[] = {
        { .opcode = BEAM_OP_RECEIVE, .arg1 = 0 },
        { .opcode = BEAM_OP_HALT }
    };

    Eterm res_val = 0;
    beam_result_t res;

    /* Execute Sender */
    res = beam_emu_execute_code(proc_sender, sender_code, sizeof(sender_code)/sizeof(sender_code[0]), &res_val);
    assert(res == BEAM_ERR_HALT);

    /* Execute Receiver */
    res = beam_emu_execute_code(proc_receiver, receiver_code, sizeof(receiver_code)/sizeof(receiver_code[0]), &res_val);
    assert(res == BEAM_ERR_HALT);
    assert(eterm_to_small_int(res_val) == 777);

    /* Test RECEIVE on empty mailbox -> Should transition to WAITING */
    beam_process_t* proc_empty = beam_process_create(503, 128, &alloc);
    res = beam_emu_execute_code(proc_empty, receiver_code, sizeof(receiver_code)/sizeof(receiver_code[0]), &res_val);
    assert(res == BEAM_OK);
    (void)res;
    assert(beam_process_get_state(proc_empty) == BEAM_PROC_STATE_WAITING);

    beam_process_destroy(proc_sender);
    beam_process_destroy(proc_receiver);
    beam_process_destroy(proc_empty);

    assert(stats.alloc_count > 0);
    assert(stats.free_count == stats.alloc_count);
    printf("  [RESULT] OP_SEND and OP_RECEIVE successfully passed value 777 and handled WAITING state!\n");
    printf("  [PASSED] test_opcode_messaging\n");
}

void test_opcode_pattern_matching(void) {
    printf("[UNIT TEST] Testing OpCode Pattern Matching (MATCH_TUPLE & GET_TUPLE_ELEMENT)...\n");

    mock_memory_stats_t stats = {0};
    beam_allocator_i alloc = mock_memory_create(&stats);

    beam_process_t* proc = beam_process_create(203, 128, &alloc);
    assert(proc != NULL);

    Eterm tuple_elems[2] = { make_small_int(88), make_small_int(99) };
    Eterm tuple_term = beam_make_tuple(proc, 2, tuple_elems);

    beam_instruction_t code[] = {
        { .opcode = BEAM_OP_MOVE,              .arg1 = 0, .arg2 = 0, .literal = tuple_term },
        { .opcode = BEAM_OP_MATCH_TUPLE,        .arg1 = 0, .arg2 = 2 },
        { .opcode = BEAM_OP_GET_TUPLE_ELEMENT, .arg1 = 0, .arg2 = 1, .arg3 = 0 },
        { .opcode = BEAM_OP_HALT }
    };

    Eterm result = 0;
    beam_result_t res = beam_emu_execute_code(proc, code, sizeof(code)/sizeof(code[0]), &result);

    assert(res == BEAM_ERR_HALT);
    (void)res;
    assert(eterm_to_small_int(result) == 99);

    beam_process_destroy(proc);

    assert(stats.alloc_count > 0);
    assert(stats.free_count == stats.alloc_count);
    printf("  [RESULT] Matched Tuple {88, 99} and extracted element 1 = 99 cleanly!\n");
    printf("  [PASSED] test_opcode_pattern_matching\n");
}

void test_opcode_select_val(void) {
    printf("[UNIT TEST] Testing OpCode Select Val (Jump Table Branching)...\n");

    mock_memory_stats_t stats = {0};
    beam_allocator_i alloc = mock_memory_create(&stats);

    beam_process_t* proc = beam_process_create(204, 128, &alloc);
    assert(proc != NULL);

    Eterm extra_pairs[4] = { make_small_int(10), 2, make_small_int(20), 3 };

    beam_instruction_t code[] = {
        { .opcode = BEAM_OP_MOVE, .arg1 = 0, .arg2 = 0, .literal = make_small_int(20) },
        { .opcode = BEAM_OP_SELECT_VAL, .arg1 = 0, .arg2 = 4, .arg3 = 2, .extra_args = extra_pairs, .extra_count = 4 },
        { .opcode = BEAM_OP_LABEL, .arg1 = 2 },
        { .opcode = BEAM_OP_MOVE, .arg1 = 0, .arg2 = 0, .literal = make_small_int(100) },
        { .opcode = BEAM_OP_HALT },
        { .opcode = BEAM_OP_LABEL, .arg1 = 3 },
        { .opcode = BEAM_OP_MOVE, .arg1 = 0, .arg2 = 0, .literal = make_small_int(200) },
        { .opcode = BEAM_OP_HALT },
        { .opcode = BEAM_OP_LABEL, .arg1 = 4 },
        { .opcode = BEAM_OP_MOVE, .arg1 = 0, .arg2 = 0, .literal = make_small_int(999) },
        { .opcode = BEAM_OP_HALT }
    };

    Eterm result = 0;
    beam_result_t res = beam_emu_execute_code(proc, code, sizeof(code)/sizeof(code[0]), &result);

    assert(res == BEAM_ERR_HALT);
    (void)res;
    assert(eterm_to_small_int(result) == 200);

    beam_process_destroy(proc);

    assert(stats.alloc_count > 0);
    assert(stats.free_count == stats.alloc_count);
    printf("  [RESULT] Matched SELECT_VAL key 20 and branched cleanly to Label 3 with result 200!\n");
    printf("  [PASSED] test_opcode_select_val\n");
}

int main(void) {
    printf("=========================================\n");
    printf(" RUNNING ISOLATED MODULE TEST: EMULATOR  \n");
    printf("=========================================\n");
    test_opcode_execution();
    test_call_stack_execution();
    test_opcode_messaging();
    test_opcode_pattern_matching();
    test_opcode_select_val();
    return 0;
}
