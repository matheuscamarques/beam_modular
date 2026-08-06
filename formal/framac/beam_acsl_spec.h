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

#endif /* BEAM_ACSL_SPEC_H */
