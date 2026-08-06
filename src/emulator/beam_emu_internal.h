#ifndef BEAM_EMU_INTERNAL_H
#define BEAM_EMU_INTERNAL_H

#include "beam_core.h"
#include "beam_scheduler.h"
#include "beam_messaging.h"

#define BEAM_NUM_X_REGISTERS 256
#define BEAM_MAX_STACK_WORDS 256

typedef enum {
    BEAM_OP_LABEL = 1,
    BEAM_OP_MOVE,
    BEAM_OP_ADD,
    BEAM_OP_SUB,
    BEAM_OP_ALLOCATE,
    BEAM_OP_DEALLOCATE,
    BEAM_OP_CALL,
    BEAM_OP_RETURN,
    BEAM_OP_SEND,
    BEAM_OP_RECEIVE,
    BEAM_OP_MATCH_TUPLE,
    BEAM_OP_GET_TUPLE_ELEMENT,
    BEAM_OP_TEST_IS_EQ_EXACT,
    BEAM_OP_TEST_IS_NE_EXACT,
    BEAM_OP_TEST_IS_TUPLE,
    BEAM_OP_TEST_IS_LIST,
    BEAM_OP_GET_LIST,
    BEAM_OP_SELECT_VAL,
    BEAM_OP_HALT
} beam_opcode_t;

struct beam_instruction {
    beam_opcode_t opcode;
    uint32_t arg1;
    uint32_t arg2;
    uint32_t arg3;
    Eterm literal;
    beam_process_t* target_proc; /* Direct target process reference for sending */
};

/* Internal register and stack state per process execution frame */
typedef struct {
    Eterm x_regs[BEAM_NUM_X_REGISTERS];
    Eterm stack[BEAM_MAX_STACK_WORDS];
    int sp;
    uint32_t cp;
    size_t ip;
} beam_emulator_frame_t;

beam_result_t beam_emu_execute_code(beam_process_t* proc, const beam_instruction_t* code, size_t code_len, Eterm* out_result);

/* Code chunk decoder */
beam_instruction_t* beam_decode_code_chunk(const uint8_t* code_chunk, size_t chunk_len, size_t* out_count, const beam_allocator_i* alloc);

#endif /* BEAM_EMU_INTERNAL_H */
