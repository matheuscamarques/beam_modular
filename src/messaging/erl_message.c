#include "erl_message_internal.h"
#include <string.h>

beam_mailbox_t* beam_mailbox_create(const beam_allocator_i* alloc) {
    if (!alloc || !alloc->alloc || !alloc->free) return NULL;

    beam_mailbox_t* mbox = (beam_mailbox_t*)alloc->alloc(alloc->ctx, sizeof(beam_mailbox_t));
    if (!mbox) return NULL;

    memset(mbox, 0, sizeof(beam_mailbox_t));
    mbox->alloc = *alloc;
    pthread_mutex_init(&mbox->lock, NULL);
    return mbox;
}

void beam_mailbox_destroy(beam_mailbox_t* mbox) {
    if (!mbox) return;

    beam_allocator_i alloc = mbox->alloc;
    pthread_mutex_destroy(&mbox->lock);
    beam_message_t* curr = mbox->head;
    while (curr) {
        beam_message_t* next = curr->next;
        alloc.free(alloc.ctx, curr);
        curr = next;
    }
    alloc.free(alloc.ctx, mbox);
}

beam_result_t beam_mailbox_enqueue(beam_mailbox_t* mbox, Eterm msg) {
    if (!mbox) return BEAM_ERR_INVALID_ARG;

    beam_message_t* node = (beam_message_t*)mbox->alloc.alloc(mbox->alloc.ctx, sizeof(beam_message_t));
    if (!node) return BEAM_ERR_NO_MEMORY;

    node->body = msg;
    atomic_init(&node->next, NULL);

    /* Lock-Free Atomic Enqueue using atomic_exchange on tail pointer */
    beam_message_t* prev_tail = atomic_exchange(&mbox->tail, node);
    if (!prev_tail) {
        mbox->head = node;
    } else {
        atomic_store(&prev_tail->next, node);
    }

    atomic_fetch_add(&mbox->atomic_count, 1);
    mbox->count++;

    return BEAM_OK;
}

beam_result_t beam_mailbox_dequeue(beam_mailbox_t* mbox, Eterm* out_msg) {
    if (!mbox || !out_msg) return BEAM_ERR_INVALID_ARG;
    if (!mbox->head) return BEAM_ERR_NOT_FOUND;

    beam_message_t* node = mbox->head;
    *out_msg = node->body;

    beam_message_t* next = atomic_load(&node->next);
    mbox->head = next;
    if (!mbox->head) {
        atomic_store(&mbox->tail, NULL);
    }
    if (mbox->count > 0) mbox->count--;
    mbox->alloc.free(mbox->alloc.ctx, node);
    return BEAM_OK;
}

void beam_mailbox_reset_save_cursor(beam_mailbox_t* mbox) {
    if (!mbox) return;
    mbox->save_cursor = mbox->head;
    mbox->save_prev = NULL;
}

beam_result_t beam_mailbox_peek_save(beam_mailbox_t* mbox, Eterm* out_msg) {
    if (!mbox || !out_msg) return BEAM_ERR_INVALID_ARG;
    if (!mbox->save_cursor) {
        /* Initialize cursor if at start */
        mbox->save_cursor = mbox->head;
        mbox->save_prev = NULL;
    }
    if (!mbox->save_cursor) return BEAM_ERR_NOT_FOUND;

    *out_msg = mbox->save_cursor->body;
    return BEAM_OK;
}

beam_result_t beam_mailbox_remove_current(beam_mailbox_t* mbox) {
    if (!mbox || !mbox->save_cursor) return BEAM_ERR_NOT_FOUND;

    beam_message_t* target = mbox->save_cursor;
    beam_message_t* next = atomic_load(&target->next);

    if (mbox->save_prev) {
        atomic_store(&mbox->save_prev->next, next);
    } else {
        mbox->head = next;
    }

    if (target == atomic_load(&mbox->tail)) {
        atomic_store(&mbox->tail, mbox->save_prev);
    }

    mbox->save_cursor = next;
    if (mbox->count > 0) mbox->count--;
    mbox->alloc.free(mbox->alloc.ctx, target);
    return BEAM_OK;
}

size_t beam_mailbox_count(const beam_mailbox_t* mbox) {
    return mbox ? mbox->count : 0;
}

beam_result_t beam_message_send_to_process(beam_process_t* receiver, Eterm msg, const beam_allocator_i* alloc) {
    beam_mailbox_t* mbox = beam_process_get_mailbox(receiver);
    if (!mbox) return BEAM_ERR_INVALID_ARG;
    (void)alloc;
    return beam_mailbox_enqueue(mbox, msg);
}

beam_result_t beam_process_receive_message(beam_process_t* proc, Eterm* out_msg) {
    beam_mailbox_t* mbox = beam_process_get_mailbox(proc);
    if (!mbox) return BEAM_ERR_INVALID_ARG;
    return beam_mailbox_dequeue(mbox, out_msg);
}
