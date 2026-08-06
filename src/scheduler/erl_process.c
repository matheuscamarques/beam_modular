#include "erl_process_internal.h"
#include <string.h>

beam_process_t* beam_process_create(uint32_t pid, size_t initial_heap_words, const beam_allocator_i* alloc) {
    if (!alloc || !alloc->alloc || !alloc->free) return NULL;

    beam_process_t* proc = (beam_process_t*)alloc->alloc(alloc->ctx, sizeof(beam_process_t));
    if (!proc) return NULL;

    memset(proc, 0, sizeof(beam_process_t));
    proc->alloc = *alloc;
    proc->pid = pid;
    proc->state = BEAM_PROC_STATE_RUNNABLE;
    proc->reductions = BEAM_DEFAULT_REDUCTIONS;

    size_t capacity = (initial_heap_words > 0) ? initial_heap_words : 256;
    proc->heap = (Eterm*)alloc->alloc(alloc->ctx, sizeof(Eterm) * capacity);
    if (!proc->heap) {
        alloc->free(alloc->ctx, proc);
        return NULL;
    }

    proc->heap_capacity = capacity;
    proc->heap_top = 0;

    proc->mailbox = beam_mailbox_create(alloc);
    if (!proc->mailbox) {
        alloc->free(alloc->ctx, proc->heap);
        alloc->free(alloc->ctx, proc);
        return NULL;
    }

    return proc;
}

void beam_process_destroy(beam_process_t* proc) {
    if (!proc) return;
    beam_allocator_i alloc = proc->alloc;

    if (proc->mailbox) {
        beam_mailbox_destroy(proc->mailbox);
    }
    if (proc->heap) {
        alloc.free(alloc.ctx, proc->heap);
    }
    alloc.free(alloc.ctx, proc);
}

uint32_t beam_process_get_pid(const beam_process_t* proc) {
    return proc ? proc->pid : 0;
}

beam_process_state_t beam_process_get_state(const beam_process_t* proc) {
    return proc ? proc->state : BEAM_PROC_STATE_EXITED;
}

void beam_process_set_state(beam_process_t* proc, beam_process_state_t state) {
    if (proc) {
        proc->state = state;
    }
}

int beam_process_get_reductions(const beam_process_t* proc) {
    return proc ? proc->reductions : 0;
}

void beam_process_set_reductions(beam_process_t* proc, int reductions) {
    if (proc) {
        proc->reductions = reductions;
    }
}

void beam_process_consume_reductions(beam_process_t* proc, int count) {
    if (proc) {
        proc->reductions -= count;
    }
}

beam_mailbox_t* beam_process_get_mailbox(beam_process_t* proc) {
    return proc ? proc->mailbox : NULL;
}

Eterm* beam_process_alloc_heap(beam_process_t* proc, size_t needed_words) {
    if (!proc) return NULL;

    if (proc->heap_top + needed_words > proc->heap_capacity) {
        size_t new_cap = proc->heap_capacity * 2 + needed_words;
        Eterm* new_heap = (Eterm*)proc->alloc.realloc(proc->alloc.ctx, proc->heap, sizeof(Eterm) * new_cap);
        if (!new_heap) return NULL;
        proc->heap = new_heap;
        proc->heap_capacity = new_cap;
    }

    Eterm* ptr = &proc->heap[proc->heap_top];
    proc->heap_top += needed_words;
    return ptr;
}

size_t beam_process_heap_used(const beam_process_t* proc) {
    return proc ? proc->heap_top : 0;
}

size_t beam_process_heap_capacity(const beam_process_t* proc) {
    return proc ? proc->heap_capacity : 0;
}

/* COMPOUND TERM CONSTRUCTORS & INSPECTORS */
Eterm beam_make_tuple(beam_process_t* proc, size_t arity, const Eterm* elements) {
    if (!proc) return ETERM_INVALID;

    Eterm* hp = beam_process_alloc_heap(proc, arity + 1);
    if (!hp) return ETERM_INVALID;

    hp[0] = (Eterm)((arity << 6) | SUBTAG_TUPLE);

    if (elements && arity > 0) {
        memcpy(&hp[1], elements, sizeof(Eterm) * arity);
    } else {
        memset(&hp[1], 0, sizeof(Eterm) * arity);
    }

    return (Eterm)((uintptr_t)hp | TAG_PRIMARY_BOXED);
}

Eterm beam_make_list(beam_process_t* proc, Eterm head, Eterm tail) {
    if (!proc) return ETERM_INVALID;

    Eterm* hp = beam_process_alloc_heap(proc, 2);
    if (!hp) return ETERM_INVALID;

    hp[0] = head;
    hp[1] = tail;

    return (Eterm)((uintptr_t)hp | TAG_PRIMARY_LIST);
}

bool beam_is_tuple(Eterm term) {
    if (!beam_is_boxed(term)) return false;
    Eterm* ptr = (Eterm*)(term & ~PRIMARY_TAG_MASK);
    return (ptr[0] & HEADER_SUBTAG_MASK) == SUBTAG_TUPLE;
}

size_t beam_tuple_arity(Eterm term) {
    if (!beam_is_tuple(term)) return 0;
    Eterm* ptr = (Eterm*)(term & ~PRIMARY_TAG_MASK);
    return (size_t)(ptr[0] >> 6);
}

Eterm beam_tuple_element(Eterm term, size_t index) {
    if (!beam_is_tuple(term)) return ETERM_INVALID;
    size_t arity = beam_tuple_arity(term);
    if (index >= arity) return ETERM_INVALID;

    Eterm* ptr = (Eterm*)(term & ~PRIMARY_TAG_MASK);
    return ptr[1 + index];
}

Eterm beam_list_head(Eterm term) {
    if (!beam_is_list(term)) return ETERM_INVALID;
    Eterm* ptr = (Eterm*)(term & ~PRIMARY_TAG_MASK);
    return ptr[0];
}

Eterm beam_list_tail(Eterm term) {
    if (!beam_is_list(term)) return ETERM_INVALID;
    Eterm* ptr = (Eterm*)(term & ~PRIMARY_TAG_MASK);
    return ptr[1];
}
