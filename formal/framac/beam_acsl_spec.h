#ifndef BEAM_ACSL_SPEC_H
#define BEAM_ACSL_SPEC_H

#include <stddef.h>
#include <stdint.h>

typedef uintptr_t Eterm;

/*@ 
  @ requires \valid(proc);
  @ requires proc->stack_top > proc->heap_top;
  @ ensures proc->stack_top == \old(proc->stack_top) - 1;
  @*/
int beam_process_stack_push_acsl(void* proc, Eterm term);

/*@ 
  @ requires \valid(proc);
  @ requires proc->stack_top > proc->heap_top + words + 1;
  @ ensures proc->stack_top == \old(proc->stack_top) - words - 1;
  @*/
int beam_op_allocate_acsl(void* proc, size_t words);

/*@ 
  @ requires \valid(proc);
  @ requires proc->stack_top + words + 1 <= proc->heap_capacity;
  @ ensures proc->stack_top == \old(proc->stack_top) + words + 1;
  @*/
int beam_op_deallocate_acsl(void* proc, size_t words);

/*@ 
  @ requires \valid(proc);
  @ requires proc->stack_top + words + 1 <= proc->heap_capacity;
  @ ensures proc->stack_top == \old(proc->stack_top) + words + 1;
  @*/
int beam_op_call_last_acsl(void* proc, size_t target_label, size_t words);

/* Exception handling (60% milestone: TRY / TRY_CASE / CATCH / TRY_END / RAISE).
   A catch frame is only pushed while the number of pending handlers is
   bounded, and every TRY_END/RAISE strictly shrinks the catch depth so the
   catch stack can never overflow or underflow. */

/*@
  @ requires \valid(proc);
  @ requires 0 <= proc->catch_depth < BEAM_MAX_CATCH_DEPTH; (already ack auth overflow bound)
  @ ensures proc->catch_depth == \old(proc->catch_depth) + 1;
  @ ensures proc->catch_ip == old(proc->frame.ip) + 1; // handler label saved
  @*/
int beam_op_try_acsl(void* proc, size_t catch_label);

/*@
  @ requires \valid(proc);
  @ requires 0 < proc->catch_depth;
  @ ensures proc->catch_depth == \old(proc->catch_depth) - 1;
  @ ensures proc->ip == proc->catch_ip; // RAISE resumes at the handler label
  @*/
int beam_op_raise_acsl(void* proc);

/*@
  @ requires \valid(proc);
  @ requires 0 < proc->catch_depth;
  @ ensures proc->catch_depth == \old(proc->catch_depth) - 1;
  @*/
int beam_op_try_end_acsl(void* proc);

#endif /* BEAM_ACSL_SPEC_H */
