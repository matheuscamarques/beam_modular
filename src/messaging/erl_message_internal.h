#ifndef ERL_MESSAGE_INTERNAL_H
#define ERL_MESSAGE_INTERNAL_H

#include "beam_messaging.h"
#include "beam_scheduler.h"

#include <pthread.h>
#include <stdatomic.h>

typedef struct beam_message beam_message_t;

struct beam_message {
    Eterm body;
    _Atomic(beam_message_t*) next;
};

struct beam_mailbox {
    beam_message_t* head;
    _Atomic(beam_message_t*) tail;
    beam_message_t* save_cursor;
    beam_message_t* save_prev;
    _Atomic(size_t) atomic_count;
    size_t count;
    pthread_mutex_t lock;
    beam_allocator_i alloc;
};

/* Selective receive pattern matching helpers */
BEAM_NODISCARD beam_result_t beam_mailbox_peek_save(beam_mailbox_t* mbox, Eterm* out_msg);
BEAM_NODISCARD beam_result_t beam_mailbox_remove_current(beam_mailbox_t* mbox);
void beam_mailbox_reset_save_cursor(beam_mailbox_t* mbox);

#endif /* ERL_MESSAGE_INTERNAL_H */
