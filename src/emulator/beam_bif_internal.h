#ifndef BEAM_BIF_INTERNAL_H
#define BEAM_BIF_INTERNAL_H

#include "beam_core.h"
#include "beam_scheduler.h"

typedef Eterm (*beam_bif_handler_t)(beam_process_t* proc, const Eterm* args, int arity);

typedef struct {
    const char* name;
    int arity;
    beam_bif_handler_t handler;
} beam_bif_entry_t;

/* Core BIF Implementations */
Eterm bif_self_0(beam_process_t* proc, const Eterm* args, int arity);
Eterm bif_add_2(beam_process_t* proc, const Eterm* args, int arity);
Eterm bif_sub_2(beam_process_t* proc, const Eterm* args, int arity);

#endif /* BEAM_BIF_INTERNAL_H */
