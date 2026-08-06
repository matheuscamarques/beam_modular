#ifndef ERL_MESSAGE_INTERNAL_H
#define ERL_MESSAGE_INTERNAL_H

#include "beam_messaging.h"
#include "beam_scheduler.h"

typedef struct beam_message beam_message_t;

struct beam_message {
    Eterm body;
    beam_message_t* next;
};

struct beam_mailbox {
    beam_message_t* head;
    beam_message_t* tail;
    size_t count;
    beam_allocator_i alloc;
};

#endif /* ERL_MESSAGE_INTERNAL_H */
