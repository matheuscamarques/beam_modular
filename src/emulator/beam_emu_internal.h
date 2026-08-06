#ifndef BEAM_EMU_INTERNAL_H
#define BEAM_EMU_INTERNAL_H

#include "beam_core.h"
#include "beam_scheduler.h"

#define BEAM_NUM_X_REGISTERS 16

typedef enum {
    BEAM_OP_LABEL = 1,
    BEAM_OP_MOVE,
    BEAM_OP_ADD,
    BEAM_OP_SUB,
    BEAM_OP_HALT
} beam_opcode_t;

struct beam_instruction {
    beam_opcode_t opcode;
    uint32_t arg1;
    uint32_t arg2;
    uint32_t arg3;
    Eterm literal;
};

/* Internal register state per process execution frame */
typedef struct {
    Eterm x_regs[BEAM_NUM_X_REGISTERS];
} beam_emulator_registers_t;

beam_result_t beam_emu_execute_code(beam_process_t* proc, const beam_instruction_t* code, size_t code_len, Eterm* out_result);

#endif /* BEAM_EMU_INTERNAL_H */
