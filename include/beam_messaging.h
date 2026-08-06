#ifndef BEAM_MESSAGING_H
#define BEAM_MESSAGING_H

#include "beam_core.h"
#include "beam_memory.h"
#include "beam_scheduler.h"

/**
 * @file beam_messaging.h
 * @brief Public Opaque Interface for Message Queue / Mailbox operations.
 */

typedef struct beam_message beam_message_t;
typedef struct beam_mailbox beam_mailbox_t;

/* Mailbox Lifecycle */
beam_mailbox_t* beam_mailbox_create(const beam_allocator_i* alloc);
void beam_mailbox_destroy(beam_mailbox_t* mbox);

/* Mailbox Operations */
beam_result_t beam_mailbox_enqueue(beam_mailbox_t* mbox, Eterm msg);
beam_result_t beam_mailbox_dequeue(beam_mailbox_t* mbox, Eterm* out_msg);
size_t beam_mailbox_count(const beam_mailbox_t* mbox);

#endif /* BEAM_MESSAGING_H */
