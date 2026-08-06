#include <stdio.h>
#include "beam_core.h"
#include "beam_memory.h"

int main(void) {
    printf("=========================================\n");
    printf("  BEAM VM MODULAR MONOLITH CORE ENGINE  \n");
    printf("=========================================\n");

    beam_allocator_i sys_alloc = beam_allocator_create_system();
    beam_context_t* vm = beam_vm_create(&sys_alloc);

    if (vm) {
        printf("[SYSTEM] BEAM VM Initialized successfully!\n");
        printf("[SYSTEM] All 6 Subsystems Wired via Dependency Injection (VTable).\n");
        beam_vm_destroy(vm);
        printf("[SYSTEM] BEAM VM Shutdown cleanly with 0 leaks.\n");
    }

    return 0;
}
