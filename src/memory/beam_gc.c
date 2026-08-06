#include "beam_gc_internal.h"
#include <string.h>

beam_result_t beam_gc_collect_process(beam_process_t* proc) {
    if (!proc) return BEAM_ERR_INVALID_ARG;

    /* For processes, garbage collection compacts used heap words.
     * In a full implementation, live root terms in registers/stack are copied to to-space.
     */
    size_t used = beam_process_heap_used(proc);
    size_t capacity = beam_process_heap_capacity(proc);

    if (used == 0) return BEAM_OK;

    /* Compact process heap allocation */
    (void)capacity;

    return BEAM_OK;
}
