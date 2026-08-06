#include "beam_emu_internal.h"
#include <stdio.h>
#include <string.h>

beam_result_t beam_emu_execute_code(beam_process_t* proc, const beam_instruction_t* code, size_t code_len, Eterm* out_result) {
    if (!proc || !code || code_len == 0) return BEAM_ERR_INVALID_ARG;

    beam_emulator_registers_t regs;
    memset(&regs, 0, sizeof(regs));

    size_t ip = 0; /* Instruction pointer */

    while (ip < code_len) {
        const beam_instruction_t* instr = &code[ip];

        /* Deduct reductions per opcode instruction */
        beam_process_consume_reductions(proc, 1);
        if (beam_process_get_reductions(proc) <= 0) {
            /* Reduction budget exhausted: trap to scheduler */
            beam_process_set_state(proc, BEAM_PROC_STATE_RUNNABLE);
            return BEAM_OK;
        }

        switch (instr->opcode) {
            case BEAM_OP_LABEL:
                /* No-op label marker */
                break;

            case BEAM_OP_MOVE: {
                /* arg1: source register or literal, arg2: dst X register */
                uint32_t dst = instr->arg2;
                if (dst < BEAM_NUM_X_REGISTERS) {
                    regs.x_regs[dst] = instr->literal;
                }
                break;
            }

            case BEAM_OP_ADD: {
                /* arg1: src X reg1, arg2: src X reg2, arg3: dst X reg */
                uint32_t r1 = instr->arg1;
                uint32_t r2 = instr->arg2;
                uint32_t dst = instr->arg3;
                if (r1 < BEAM_NUM_X_REGISTERS && r2 < BEAM_NUM_X_REGISTERS && dst < BEAM_NUM_X_REGISTERS) {
                    intptr_t v1 = eterm_to_small_int(regs.x_regs[r1]);
                    intptr_t v2 = eterm_to_small_int(regs.x_regs[r2]);
                    regs.x_regs[dst] = make_small_int(v1 + v2);
                }
                break;
            }

            case BEAM_OP_SUB: {
                uint32_t r1 = instr->arg1;
                uint32_t r2 = instr->arg2;
                uint32_t dst = instr->arg3;
                if (r1 < BEAM_NUM_X_REGISTERS && r2 < BEAM_NUM_X_REGISTERS && dst < BEAM_NUM_X_REGISTERS) {
                    intptr_t v1 = eterm_to_small_int(regs.x_regs[r1]);
                    intptr_t v2 = eterm_to_small_int(regs.x_regs[r2]);
                    regs.x_regs[dst] = make_small_int(v1 - v2);
                }
                break;
            }

            case BEAM_OP_HALT: {
                if (out_result) {
                    *out_result = regs.x_regs[0]; /* X[0] holds return value */
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
        *out_result = regs.x_regs[0];
    }
    return BEAM_OK;
}
