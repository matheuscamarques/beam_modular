#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "beam_core.h"
#include "beam_scheduler.h"
#include "beam_emu_internal.h"
#include "mock_memory.h"

void test_opcode_execution(void) {
    printf("[UNIT TEST] Testing OpCode Interpreter Execution...\n");

    mock_memory_stats_t stats = {0};
    beam_allocator_i alloc = mock_memory_create(&stats);

    beam_process_t* proc = beam_process_create(201, 128, &alloc);
    assert(proc != NULL);

    /* Construct Bytecode:
     * 1. MOVE 10 -> X[0]
     * 2. MOVE 20 -> X[1]
     * 3. ADD X[0], X[1] -> X[0]
     * 4. HALT
     */
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

    /* Bytecode:
     * 0: ALLOCATE 1
     * 1: MOVE 100 -> X[0]
     * 2: CALL (target = 5)
     * 3: DEALLOCATE 1
     * 4: HALT
     * --- Target Function ---
     * 5: MOVE 50 -> X[1]
     * 6: ADD X[0], X[1] -> X[0]
     * 7: RETURN
     */
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

int main(void) {
    printf("=========================================\n");
    printf(" RUNNING ISOLATED MODULE TEST: EMULATOR  \n");
    printf("=========================================\n");
    test_opcode_execution();
    test_call_stack_execution();
    return 0;
}
