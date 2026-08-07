#include "beam_gc_internal.h"
#include "../scheduler/erl_process_internal.h"
#include <string.h>

static Eterm gc_copy_term(Eterm term, Eterm* to_space, size_t* to_top, size_t to_cap) {
    /* If term is an immediate (small int, atom, nil), copy directly without allocation */
    if (beam_is_small_int(term) || beam_is_atom(term) || beam_is_nil(term)) {
        return term;
    }

    /* Heap tuple copying */
    if (beam_is_tuple(term)) {
        size_t arity = beam_tuple_arity(term);
        size_t words_needed = arity + 1;
        if (*to_top + words_needed > to_cap) return term;

        size_t new_idx = *to_top;
        to_space[new_idx] = make_small_int((intptr_t)arity);
        *to_top += words_needed;

        for (size_t i = 0; i < arity; i++) {
            Eterm elem = beam_tuple_element(term, i);
            to_space[new_idx + 1 + i] = gc_copy_term(elem, to_space, to_top, to_cap);
        }
        return (Eterm)(((uintptr_t)&to_space[new_idx]) | TAG_PRIMARY_BOXED);
    }

    /* Heap list cell copying */
    if (beam_is_list(term)) {
        if (*to_top + 2 > to_cap) return term;

        size_t new_idx = *to_top;
        Eterm head = beam_list_head(term);
        Eterm tail = beam_list_tail(term);
        *to_top += 2;

        to_space[new_idx]     = gc_copy_term(head, to_space, to_top, to_cap);
        to_space[new_idx + 1] = gc_copy_term(tail, to_space, to_top, to_cap);

        return (Eterm)(((uintptr_t)&to_space[new_idx]) | TAG_PRIMARY_LIST);
    }

    return term;
}

beam_result_t beam_gc_collect_process(beam_process_t* proc) {
    if (!proc || !proc->heap) return BEAM_ERR_INVALID_ARG;

    size_t old_cap = proc->heap_capacity;
    size_t new_cap = old_cap;

    /* Allocate To-Space heap */
    Eterm* to_space = (Eterm*)proc->alloc.alloc(proc->alloc.ctx, sizeof(Eterm) * new_cap);
    if (!to_space) return BEAM_ERR_NO_MEMORY;

    size_t to_heap_top = 0;

    /* 1. Copy Roots: X Registers */
    for (size_t i = 0; i < BEAM_NUM_X_REGISTERS; i++) {
        if (proc->frame.x_regs[i] != 0) {
            proc->frame.x_regs[i] = gc_copy_term(proc->frame.x_regs[i], to_space, &to_heap_top, new_cap);
        }
    }

    /* 2. Copy Roots: Unified Stack Slots */
    size_t new_stack_top = new_cap - (old_cap - proc->stack_top);
    for (size_t s = proc->stack_top; s < old_cap; s++) {
        size_t offset_from_bottom = old_cap - s;
        size_t to_stack_idx = new_cap - offset_from_bottom;
        to_space[to_stack_idx] = gc_copy_term(proc->heap[s], to_space, &to_heap_top, new_cap);
    }

    /* Swap From-Space with To-Space */
    proc->alloc.free(proc->alloc.ctx, proc->heap);
    proc->heap = to_space;
    proc->heap_capacity = new_cap;
    proc->heap_top = to_heap_top;
    proc->stack_top = new_stack_top;

    return BEAM_OK;
}
