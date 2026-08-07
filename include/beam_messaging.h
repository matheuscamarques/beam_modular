#ifndef BEAM_MESSAGING_H
#define BEAM_MESSAGING_H

#include "beam_core.h"
#include "beam_memory.h"

/**
 * @file beam_messaging.h
 * @brief Public Opaque Interface for Mailbox & Message Passing Subsystem (C23 ISO Standard).
 */

typedef struct beam_mailbox beam_mailbox_t;

BEAM_NODISCARD beam_mailbox_t* beam_mailbox_create(const beam_allocator_i* alloc);
void beam_mailbox_destroy(beam_mailbox_t* mbox);

BEAM_NODISCARD beam_result_t beam_mailbox_enqueue(beam_mailbox_t* mbox, Eterm message);
BEAM_NODISCARD beam_result_t beam_mailbox_dequeue(beam_mailbox_t* mbox, Eterm* out_message);
size_t beam_mailbox_count(const beam_mailbox_t* mbox);

/* High-level Process-to-Process Messaging */
BEAM_NODISCARD beam_result_t beam_message_send_to_process(beam_process_t* target_proc, Eterm message, const beam_allocator_i* alloc);
void beam_process_reset_mailbox_cursor(beam_process_t* proc);

#endif /* BEAM_MESSAGING_H */
