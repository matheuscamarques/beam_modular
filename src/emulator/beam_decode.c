#include "beam_emu_internal.h"
#include "beam_opcodes_arity.h"
#include <string.h>

static uint32_t read_u32_be(const uint8_t* ptr) {
    return ((uint32_t)ptr[0] << 24) |
           ((uint32_t)ptr[1] << 16) |
           ((uint32_t)ptr[2] << 8)  |
            (uint32_t)ptr[3];
}

static Eterm decode_arg(const uint8_t** ptr, const uint8_t* end) {
    const uint8_t* p = *ptr;
    if (p >= end) return 0;
    uint8_t b = *p++;
    uint8_t tag = b & 0x07;
    uint32_t val;
    
    if ((b & 0x08) == 0) {
        val = b >> 4;
    } else if ((b & 0x10) == 0) {
        if (p >= end) return 0;
        val = ((b >> 5) << 8) | *p++;
    } else {
        uint32_t len = b >> 5;
        if (len < 7) {
            len += 2;
        } else {
            /* Nested length encoding */
            const uint8_t* nested = p;
            Eterm len_term = decode_arg(&nested, end);
            len = eterm_to_small_int(len_term); /* Simplification */
            p = nested;
        }
        if (len > 1024) len = 1024; // safety bound
        val = 0;
        for (uint32_t i = 0; i < len; i++) {
            if (p >= end) break;
            val = (val << 8) | *p++;
        }
    }
    
    *ptr = p;
    
    switch (tag) {
        case 0: /* Literal/Unsigned */
            return make_small_int(val);
        case 1: /* Integer */
            return make_small_int(val);
        case 2: /* Atom */
            return (val << 4) | TAG_IMMED1_ATOM;
        case 3: /* X Register */
            return val; /* Return raw index for X reg */
        case 4: /* Y Register */
            return val; /* Return raw index for Y reg */
        case 5: /* Label */
            return val;
        case 6: /* Char */
            return make_small_int(val);
        case 7: /* Extended */
            /* Simplified handling for extended tags */
            return val;
    }
    return 0;
}

beam_instruction_t* beam_decode_code_chunk(const uint8_t* code_chunk, size_t chunk_len, size_t* out_count, const beam_allocator_i* alloc) {
    if (!code_chunk || chunk_len < 20 || !alloc || !out_count) return NULL;
    
    uint32_t sub_size = read_u32_be(&code_chunk[0]);
    // uint32_t instruction_set = read_u32_be(&code_chunk[4]);
    // uint32_t opcode_max = read_u32_be(&code_chunk[8]);
    // uint32_t label_count = read_u32_be(&code_chunk[12]);
    // uint32_t function_count = read_u32_be(&code_chunk[16]);
    
    const uint8_t* p = &code_chunk[4 + sub_size];
    const uint8_t* end = code_chunk + chunk_len;
    
    /* First pass: count instructions to allocate */
    size_t count = 0;
    const uint8_t* scan = p;
    while (scan < end) {
        (void)*scan++;
        /* Simple skip logic for arguments based on opcode... */
        /* Since BEAM bytecodes are variable length, an accurate scan requires full parsing. */
        /* For the modular MVP, we will over-allocate or use a dynamic array */
        count++;
        /* Actually, without knowing the arity of each opcode, we can't easily skip. */
        /* We must fully decode in one pass or pre-allocate a safe upper bound. */
        /* The number of bytes is a safe upper bound for instruction count. */
    }
    
    beam_instruction_t* instrs = (beam_instruction_t*)alloc->alloc(alloc->ctx, sizeof(beam_instruction_t) * (chunk_len));
    if (!instrs) return NULL;
    
    size_t i = 0;
    size_t safety_counter = 0;
    while (p < end) {
        if (safety_counter++ > 100000) {
            break;
        }
        uint8_t opcode = *p++;
        beam_instruction_t* instr = &instrs[i++];
        memset(instr, 0, sizeof(beam_instruction_t));
        
        switch (opcode) {
            case 1: /* label */
                instr->opcode = BEAM_OP_LABEL;
                instr->arg1 = decode_arg(&p, end);
                break;
            case 64: /* move */
                instr->opcode = BEAM_OP_MOVE;
                instr->arg1 = decode_arg(&p, end); // Source
                instr->arg2 = decode_arg(&p, end); // Dest
                break;
            case 4: /* call */
                instr->opcode = BEAM_OP_CALL;
                instr->arg1 = decode_arg(&p, end); // Arity
                instr->arg2 = decode_arg(&p, end); // Label
                break;
            case 19: /* return */
                instr->opcode = BEAM_OP_RETURN;
                break;
            case 153: /* line */
                instr->opcode = 153; /* placeholder */
                instr->arg1 = decode_arg(&p, end);
                break;
            case 2: /* func_info */
                instr->opcode = 2;
                instr->arg1 = decode_arg(&p, end);
                instr->arg2 = decode_arg(&p, end);
                instr->arg3 = decode_arg(&p, end);
                break;
            case 3: /* int_code_end */
                instr->opcode = 3;
                break;
            case 16: /* test_heap */
                instr->opcode = 16;
                instr->arg1 = decode_arg(&p, end);
                instr->arg2 = decode_arg(&p, end);
                break;
            case 69: /* get_list */
                instr->opcode = BEAM_OP_GET_LIST;
                instr->arg1 = decode_arg(&p, end);
                instr->arg2 = decode_arg(&p, end);
                instr->arg3 = decode_arg(&p, end);
                break;
            case 78: /* select_val */
                instr->opcode = BEAM_OP_SELECT_VAL;
                instr->arg1 = decode_arg(&p, end);
                instr->arg2 = decode_arg(&p, end);
                instr->arg3 = decode_arg(&p, end); /* extended list arg */
                break;
            case 62: /* catch */
                instr->opcode = BEAM_OP_CATCH;
                instr->arg1 = decode_arg(&p, end); // Catch label
                instr->arg2 = decode_arg(&p, end); // Stack words
                break;
            case 63: /* catch_end */
                instr->opcode = BEAM_OP_TRY_END;
                instr->arg1 = decode_arg(&p, end); // Deallocate words
                break;
            case 104: /* try FtVJ*/
                instr->opcode = BEAM_OP_TRY;
                instr->arg1 = decode_arg(&p, end); // Catch label
                instr->arg2 = decode_arg(&p, end); // Value register
                break;
            case 105: /* try_end */
                instr->opcode = BEAM_OP_TRY_END;
                instr->arg1 = decode_arg(&p, end); // Deallocate words
                break;
            case 106: /* try_case */
                instr->opcode = BEAM_OP_TRY_CASE;
                instr->arg1 = decode_arg(&p, end);
                break;
            case 107: /* try_case_end */
                instr->opcode = BEAM_OP_TRY_CASE_END;
                instr->arg1 = decode_arg(&p, end);
                break;
            case 108: /* raise */
                instr->opcode = BEAM_OP_RAISE;
                instr->arg1 = decode_arg(&p, end); // Trace
                instr->arg2 = decode_arg(&p, end); // Value register
                break;
            case 161: /* raw_raise */
                instr->opcode = BEAM_OP_RAISE;
                break;
            /* Add more opcodes as needed */
            default: {
                /* Unknown opcode - skip its arguments based on arity table */
                uint8_t arity = beam_opcode_arities[opcode];
                for (uint8_t a = 0; a < arity; a++) {
                    decode_arg(&p, end);
                }
                break;
            }
        }
    }
    
    *out_count = i;
    return instrs;
}
