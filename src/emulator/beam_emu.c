#include "beam_emu_internal.h"
#include <stdio.h>
#include <string.h>

beam_result_t beam_emu_execute_code(beam_process_t* proc, const beam_instruction_t* code, size_t code_len, Eterm* out_result) {
    if (!proc || !code || code_len == 0) return BEAM_ERR_INVALID_ARG;

    beam_emulator_frame_t frame;
    memset(&frame, 0, sizeof(frame));
    frame.sp = BEAM_MAX_STACK_WORDS;
    frame.cp = (uint32_t)code_len;

    size_t ip = 0;

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
                    frame.x_regs[dst] = instr->literal;
                }
                break;
            }

            case BEAM_OP_ADD: {
                uint32_t r1 = instr->arg1;
                uint32_t r2 = instr->arg2;
                uint32_t dst = instr->arg3;
                if (r1 < BEAM_NUM_X_REGISTERS && r2 < BEAM_NUM_X_REGISTERS && dst < BEAM_NUM_X_REGISTERS) {
                    intptr_t v1 = eterm_to_small_int(frame.x_regs[r1]);
                    intptr_t v2 = eterm_to_small_int(frame.x_regs[r2]);
                    frame.x_regs[dst] = make_small_int(v1 + v2);
                }
                break;
            }

            case BEAM_OP_SUB: {
                uint32_t r1 = instr->arg1;
                uint32_t r2 = instr->arg2;
                uint32_t dst = instr->arg3;
                if (r1 < BEAM_NUM_X_REGISTERS && r2 < BEAM_NUM_X_REGISTERS && dst < BEAM_NUM_X_REGISTERS) {
                    intptr_t v1 = eterm_to_small_int(frame.x_regs[r1]);
                    intptr_t v2 = eterm_to_small_int(frame.x_regs[r2]);
                    frame.x_regs[dst] = make_small_int(v1 - v2);
                }
                break;
            }

            case BEAM_OP_ALLOCATE: {
                uint32_t words = instr->arg1;
                if (frame.sp - (int)words - 1 < 0) {
                    return BEAM_ERR_NO_MEMORY;
                }
                frame.sp--;
                frame.stack[frame.sp] = (Eterm)frame.cp;
                frame.sp -= (int)words;
                break;
            }

            case BEAM_OP_DEALLOCATE: {
                uint32_t words = instr->arg1;
                frame.sp += (int)words;
                frame.cp = (uint32_t)frame.stack[frame.sp];
                frame.sp++;
                break;
            }

            case BEAM_OP_CALL: {
                frame.cp = (uint32_t)(ip + 1);
                ip = (size_t)instr->arg1;
                continue;
            }

            case BEAM_OP_RETURN: {
                ip = (size_t)frame.cp;
                continue;
            }

            case BEAM_OP_SEND: {
                /* arg1: msg register index, target_proc: receiver process pointer */
                uint32_t msg_reg = instr->arg1;
                beam_process_t* target = instr->target_proc;
                if (msg_reg < BEAM_NUM_X_REGISTERS && target) {
                    Eterm msg = frame.x_regs[msg_reg];
                    beam_message_send_to_process(target, msg, NULL);
                }
                break;
            }

            case BEAM_OP_RECEIVE: {
                /* arg1: dst register index */
                uint32_t dst_reg = instr->arg1;
                Eterm msg = 0;
                beam_result_t res = beam_process_receive_message(proc, &msg);
                if (res == BEAM_OK) {
                    if (dst_reg < BEAM_NUM_X_REGISTERS) {
                        frame.x_regs[dst_reg] = msg;
                    }
                } else {
                    /* Mailbox empty: yield CPU and set WAITING state */
                    beam_process_set_state(proc, BEAM_PROC_STATE_WAITING);
                    return BEAM_OK;
                }
                break;
            }

            case BEAM_OP_MATCH_TUPLE: {
                /* arg1: tuple reg, arg2: expected arity */
                uint32_t src_reg = instr->arg1;
                uint32_t expected_arity = instr->arg2;
                if (src_reg < BEAM_NUM_X_REGISTERS) {
                    Eterm val = frame.x_regs[src_reg];
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
                    Eterm tuple_val = frame.x_regs[src_reg];
                    frame.x_regs[dst_reg] = beam_tuple_element(tuple_val, elem_idx);
                }
                break;
            }

            case BEAM_OP_HALT: {
                if (out_result) {
                    *out_result = frame.x_regs[0];
                }
                beam_process_set_state(proc, BEAM_PROC_STATE_EXITED);
                return BEAM_ERR_HALT;
            }

            default:
                return BEAM_ERR_BADARG;
        }

        ip++;
    }

    if (out_result) {
        *out_result = frame.x_regs[0];
    }
    return BEAM_OK;
}
