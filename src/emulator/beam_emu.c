#include "beam_emu_internal.h"
#include "beam_bif_internal.h"
#include "../scheduler/erl_process_internal.h"
#include "../messaging/erl_message_internal.h"
#include <stdio.h>
#include <string.h>

beam_result_t beam_emu_execute_code(beam_process_t* proc, const beam_instruction_t* code, size_t code_len, Eterm* out_result) {
    if (!proc || !code || code_len == 0) return BEAM_ERR_INVALID_ARG;

    beam_emulator_frame_t* frame = &proc->frame;
    if (frame->cp == 0 && frame->ip == 0) {
        frame->cp = (uint32_t)code_len; // Initial CP
    }

    size_t ip = frame->ip;

#define JUMP_TO_LABEL(lbl) \
    do { \
        for (size_t scan_ip = 0; scan_ip < code_len; scan_ip++) { \
            if (code[scan_ip].opcode == BEAM_OP_LABEL && code[scan_ip].arg1 == (lbl)) { \
                ip = scan_ip; \
                break; \
            } \
        } \
    } while(0)

    while (ip < code_len) {
        const beam_instruction_t* instr = &code[ip];

        /* Deduct reductions per opcode instruction */
        beam_process_consume_reductions(proc, 1);
        if (beam_process_get_reductions(proc) <= 0) {
            beam_process_set_state(proc, BEAM_PROC_STATE_RUNNABLE);
            return BEAM_OK;
        }

        switch (instr->opcode) {
            case BEAM_OP_LABEL:
                break;

            case BEAM_OP_MOVE: {
                uint32_t dst = instr->arg2;
                if (dst < BEAM_NUM_X_REGISTERS) {
                    frame->x_regs[dst] = instr->literal;
                }
                break;
            }

            case BEAM_OP_ADD: {
                uint32_t r1 = instr->arg1;
                uint32_t r2 = instr->arg2;
                uint32_t dst = instr->arg3;
                if (r1 < BEAM_NUM_X_REGISTERS && r2 < BEAM_NUM_X_REGISTERS && dst < BEAM_NUM_X_REGISTERS) {
                    intptr_t v1 = eterm_to_small_int(frame->x_regs[r1]);
                    intptr_t v2 = eterm_to_small_int(frame->x_regs[r2]);
                    frame->x_regs[dst] = make_small_int(v1 + v2);
                }
                break;
            }

            case BEAM_OP_SUB: {
                uint32_t r1 = instr->arg1;
                uint32_t r2 = instr->arg2;
                uint32_t dst = instr->arg3;
                if (r1 < BEAM_NUM_X_REGISTERS && r2 < BEAM_NUM_X_REGISTERS && dst < BEAM_NUM_X_REGISTERS) {
                    intptr_t v1 = eterm_to_small_int(frame->x_regs[r1]);
                    intptr_t v2 = eterm_to_small_int(frame->x_regs[r2]);
                    frame->x_regs[dst] = make_small_int(v1 - v2);
                }
                break;
            }

            case BEAM_OP_MUL: {
                uint32_t r1 = instr->arg1;
                uint32_t r2 = instr->arg2;
                uint32_t dst = instr->arg3;
                if (r1 < BEAM_NUM_X_REGISTERS && r2 < BEAM_NUM_X_REGISTERS && dst < BEAM_NUM_X_REGISTERS) {
                    intptr_t v1 = eterm_to_small_int(frame->x_regs[r1]);
                    intptr_t v2 = eterm_to_small_int(frame->x_regs[r2]);
                    frame->x_regs[dst] = make_small_int(v1 * v2);
                }
                break;
            }

            case BEAM_OP_INT_DIV: {
                uint32_t r1 = instr->arg1;
                uint32_t r2 = instr->arg2;
                uint32_t dst = instr->arg3;
                if (r1 < BEAM_NUM_X_REGISTERS && r2 < BEAM_NUM_X_REGISTERS && dst < BEAM_NUM_X_REGISTERS) {
                    intptr_t v1 = eterm_to_small_int(frame->x_regs[r1]);
                    intptr_t v2 = eterm_to_small_int(frame->x_regs[r2]);
                    if (v2 == 0) return BEAM_ERR_BADARG; /* Badarith / Division by zero error */
                    frame->x_regs[dst] = make_small_int(v1 / v2);
                }
                break;
            }

            case BEAM_OP_ALLOCATE: {
                uint32_t words = instr->arg1;
                /* Save CP onto unified stack */
                beam_result_t push_res = beam_process_stack_push(proc, (Eterm)frame->cp);
                if (push_res != BEAM_OK) {
                    return push_res;
                }
                /* Reserve Y registers / words on unified stack */
                for (uint32_t w = 0; w < words; w++) {
                    push_res = beam_process_stack_push(proc, 0);
                    if (push_res != BEAM_OK) return push_res;
                }
                break;
            }

            case BEAM_OP_DEALLOCATE: {
                uint32_t words = instr->arg1;
                Eterm dummy = 0;
                /* Pop allocated Y register words */
                for (uint32_t w = 0; w < words; w++) {
                    beam_result_t pop_dummy = beam_process_stack_pop(proc, &dummy);
                    if (pop_dummy != BEAM_OK) return pop_dummy;
                }
                /* Restore CP from unified stack */
                Eterm restored_cp = 0;
                beam_result_t pop_res = beam_process_stack_pop(proc, &restored_cp);
                if (pop_res != BEAM_OK) {
                    return pop_res;
                }
                frame->cp = (uint32_t)restored_cp;
                break;
            }

            case BEAM_OP_CALL: {
                frame->cp = (uint32_t)(ip + 1);
                ip = (size_t)instr->arg1;
                continue;
            }

            case BEAM_OP_CALL_LAST: {
                /* Tail Call Optimization (TCO): Deallocate frame and jump directly without updating CP */
                uint32_t words = instr->arg2;
                Eterm dummy = 0;
                for (uint32_t w = 0; w < words; w++) {
                    beam_result_t pop_dummy = beam_process_stack_pop(proc, &dummy);
                    if (pop_dummy != BEAM_OK) return pop_dummy;
                }
                Eterm restored_cp = 0;
                beam_result_t pop_res = beam_process_stack_pop(proc, &restored_cp);
                if (pop_res != BEAM_OK) return pop_res;
                frame->cp = (uint32_t)restored_cp;
                
                /* Jump directly to target function without adding a new return address */
                ip = (size_t)instr->arg1;
                continue;
            }

            case BEAM_OP_RETURN: {
                ip = (size_t)frame->cp;
                continue;
            }

            case BEAM_OP_SEND: {
                /* arg1: msg register index, target_proc: receiver process pointer */
                uint32_t msg_reg = instr->arg1;
                beam_process_t* target = instr->target_proc;
                if (msg_reg < BEAM_NUM_X_REGISTERS && target) {
                    Eterm msg = frame->x_regs[msg_reg];
                    beam_result_t send_res = beam_message_send_to_process(target, msg, NULL);
                    (void)send_res;
                }
                break;
            }

            case BEAM_OP_LOOP_REC: {
                /* arg1: fail_label, arg2: dst_reg */
                uint32_t fail_label = instr->arg1;
                uint32_t dst_reg = instr->arg2;
                Eterm msg = 0;
                beam_result_t res = beam_mailbox_peek_save(proc->mailbox, &msg);
                if (res == BEAM_OK) {
                    if (dst_reg < BEAM_NUM_X_REGISTERS) {
                        frame->x_regs[dst_reg] = msg;
                    }
                } else {
                    JUMP_TO_LABEL(fail_label);
                    continue;
                }
                break;
            }

            case BEAM_OP_REMOVE_MESSAGE: {
                beam_result_t rm_res = beam_mailbox_remove_current(proc->mailbox);
                (void)rm_res;
                break;
            }

            case BEAM_OP_LOOP_REC_END: {
                /* Advance save cursor to next message in mailbox */
                if (proc->mailbox->save_cursor) {
                    proc->mailbox->save_prev = proc->mailbox->save_cursor;
                    proc->mailbox->save_cursor = proc->mailbox->save_cursor->next;
                }
                uint32_t loop_label = instr->arg1;
                JUMP_TO_LABEL(loop_label);
                continue;
            }

case BEAM_OP_WAIT: {
                /* Yield process and wait for new incoming messages */
                frame->ip = ip;
                beam_process_set_state(proc, BEAM_PROC_STATE_WAITING);
                return BEAM_OK;
            }

case BEAM_OP_TRY:
            case BEAM_OP_CATCH: {
                /* arg1: catch label; push a catch frame so RAISE unwinds here */
                if (frame->catch_depth >= BEAM_MAX_CATCH_DEPTH) {
                    return BEAM_ERR_NO_MEMORY;
                }
                struct beam_catch_frame* cf = &frame->catch_stack[frame->catch_depth++];
                cf->catch_label = instr->arg1;
                cf->catch_stack_top = proc->stack_top;
                cf->catch_cp = frame->cp;
                frame->catch_ip = instr->arg1;
                frame->catch_sp = proc->stack_top;
                break;
            }

            case BEAM_OP_TRY_CASE:
            case BEAM_OP_TRY_CASE_END: {
                /* Enter/exit the catch-clause region; alignment marker no-op */
                break;
            }

            case BEAM_OP_TRY_END: {
                /* Clean exit from the try block: pop innermost catch frame */
                if (frame->catch_depth > 0) {
                    frame->catch_depth--;
                    struct beam_catch_frame* prev = NULL;
                    if (frame->catch_depth > 0) {
                        prev = &frame->catch_stack[frame->catch_depth - 1];
                    }
                    frame->catch_ip = prev ? prev->catch_label : 0;
                    frame->catch_sp = prev ? prev->catch_stack_top : 0;
                }
                break;
            }

            case BEAM_OP_RAISE: {
                /* arg1: X register holding the exception term */
                Eterm exc = (instr->arg1 < BEAM_NUM_X_REGISTERS) ? frame->x_regs[instr->arg1] : 0;
                if (frame->catch_depth == 0) {
                    /* Uncaught exception terminates the process */
                    beam_process_set_state(proc, BEAM_PROC_STATE_EXITED);
                    return BEAM_ERR_EXCEPTION;
                }
                struct beam_catch_frame* cf = &frame->catch_stack[frame->catch_depth - 1];
                size_t label = cf->catch_label;
                proc->stack_top = cf->catch_stack_top; /* unwind stack frames */
                frame->catch_depth--;
                frame->x_regs[0] = exc; /* deliver term to catch clause */
                JUMP_TO_LABEL(label);
                continue;
            }

            case BEAM_OP_MATCH_TUPLE: {
                /* arg1: tuple reg, arg2: expected arity */
                uint32_t src_reg = instr->arg1;
                uint32_t expected_arity = instr->arg2;
                if (src_reg < BEAM_NUM_X_REGISTERS) {
                    Eterm val = frame->x_regs[src_reg];
                    if (!beam_is_tuple(val) || beam_tuple_arity(val) != (size_t)expected_arity) {
                        return BEAM_ERR_BADARG;
                    }
                }
                break;
            }

            case BEAM_OP_GET_TUPLE_ELEMENT: {
                /* arg1: tuple reg, arg2: elem index, arg3: dst reg */
                uint32_t src_reg = instr->arg1;
                uint32_t elem_idx = instr->arg2;
                uint32_t dst_reg = instr->arg3;
                if (src_reg < BEAM_NUM_X_REGISTERS && dst_reg < BEAM_NUM_X_REGISTERS) {
                    Eterm tuple_val = frame->x_regs[src_reg];
                    frame->x_regs[dst_reg] = beam_tuple_element(tuple_val, elem_idx);
                }
                break;
            }

            case BEAM_OP_TEST_IS_EQ_EXACT: {
                /* arg1: fail_label, arg2: reg1, arg3: reg2 */
                uint32_t fail_label = instr->arg1;
                uint32_t r1 = instr->arg2;
                uint32_t r2 = instr->arg3;
                if (frame->x_regs[r1] != frame->x_regs[r2]) {
                    JUMP_TO_LABEL(fail_label);
                    continue;
                }
                break;
            }

            case BEAM_OP_TEST_IS_NE_EXACT: {
                uint32_t fail_label = instr->arg1;
                uint32_t r1 = instr->arg2;
                uint32_t r2 = instr->arg3;
                if (frame->x_regs[r1] == frame->x_regs[r2]) {
                    JUMP_TO_LABEL(fail_label);
                    continue;
                }
                break;
            }

            case BEAM_OP_TEST_IS_TUPLE: {
                uint32_t fail_label = instr->arg1;
                uint32_t r1 = instr->arg2;
                if (!beam_is_tuple(frame->x_regs[r1])) {
                    JUMP_TO_LABEL(fail_label);
                    continue;
                }
                break;
            }

            case BEAM_OP_TEST_IS_LIST: {
                uint32_t fail_label = instr->arg1;
                uint32_t r1 = instr->arg2;
                if (!beam_is_list(frame->x_regs[r1]) && !beam_is_nil(frame->x_regs[r1])) {
                    JUMP_TO_LABEL(fail_label);
                    continue;
                }
                break;
            }

            case BEAM_OP_GET_LIST: {
                /* arg1: src list reg, arg2: dst head reg, arg3: dst tail reg */
                uint32_t src = instr->arg1;
                uint32_t head = instr->arg2;
                uint32_t tail = instr->arg3;
                Eterm list = frame->x_regs[src];
                if (beam_is_list(list)) {
                    frame->x_regs[head] = beam_list_head(list);
                    frame->x_regs[tail] = beam_list_tail(list);
                } else {
                    return BEAM_ERR_BADARG;
                }
                break;
            }

            case BEAM_OP_SELECT_VAL: {
                /* arg1: src reg, arg2: fail_label, arg3: jump_table_size */
                uint32_t src_reg = instr->arg1;
                uint32_t fail_label = instr->arg2;
                
                /* Select matching target label or jump to fail_label if not matched */
                bool matched = false;
                if (instr->extra_args && instr->extra_count >= 2) {
                    for (size_t k = 0; k < instr->extra_count; k += 2) {
                        if (frame->x_regs[src_reg] == instr->extra_args[k]) {
                            uint32_t target_label = (uint32_t)instr->extra_args[k + 1];
                            JUMP_TO_LABEL(target_label);
                            matched = true;
                            break;
                        }
                    }
                }
                
                if (!matched) {
                    JUMP_TO_LABEL(fail_label);
                }
                continue;
            }

            case BEAM_OP_CALL_EXT: {
                /* arg1: bif_index, arg2: arity */
                size_t bif_index = (size_t)instr->arg1;
                int arity = (int)instr->arg2;
                Eterm bif_res = 0;
                beam_result_t res = beam_bif_dispatch(bif_index, proc, frame->x_regs, arity, &bif_res);
                if (res == BEAM_OK) {
                    frame->x_regs[0] = bif_res;
                } else {
                    return res;
                }
                break;
            }

            case BEAM_OP_MAKE_FUN2: {
                /* arg1: fun entry label, arg2: num_free_vars, arg3: dst_reg */
                uint32_t fun_label = instr->arg1;
                uint32_t num_free = instr->arg2;
                uint32_t dst_reg = instr->arg3;
                
                /* Allocate Closure Header + Entry Label + Free Environment Variables on Process Heap */
                Eterm* hp = beam_process_alloc_heap(proc, num_free + 2);
                if (!hp) return BEAM_ERR_NO_MEMORY;
                
                hp[0] = (Eterm)((num_free + 1) << 6 | SUBTAG_TUPLE); /* Header tuple tag for Closure */
                hp[1] = make_small_int(fun_label);
                for (uint32_t fv = 0; fv < num_free; fv++) {
                    hp[2 + fv] = (fv < BEAM_NUM_X_REGISTERS) ? frame->x_regs[fv] : 0;
                }
                
                if (dst_reg < BEAM_NUM_X_REGISTERS) {
                    frame->x_regs[dst_reg] = (Eterm)((uintptr_t)hp | TAG_PRIMARY_BOXED);
                }
                break;
            }

            case BEAM_OP_HALT: {
                if (out_result) {
                    *out_result = frame->x_regs[0];
                }
                beam_process_set_state(proc, BEAM_PROC_STATE_EXITED);
                return BEAM_ERR_HALT;
            }

            default:
                return BEAM_ERR_BADARG;
        }

        ip++;
    }
#undef JUMP_TO_LABEL

    if (out_result) {
        *out_result = frame->x_regs[0];
    }
    frame->ip = ip;
    return BEAM_OK;
}
