#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "beam_core.h"
#include "beam_memory.h"

void test_system_allocator(void) {
    printf("[UNIT TEST] Testing System Allocator...\n");

    beam_allocator_i alloc = beam_allocator_create_system();
    assert(alloc.alloc != NULL);
    assert(alloc.free != NULL);
    assert(alloc.ctx != NULL);

    int* ptr = (int*)alloc.alloc(alloc.ctx, sizeof(int) * 10);
    assert(ptr != NULL);

    for (int i = 0; i < 10; i++) {
        ptr[i] = i * 42;
    }
    assert(ptr[9] == 378);

    beam_memory_stats_t stats = beam_allocator_get_stats(&alloc);
    assert(stats.active_allocations == 1);
    assert(stats.total_allocated_bytes >= sizeof(int) * 10);
    (void)stats;

    alloc.free(alloc.ctx, ptr);

    printf("[PASSED] test_system_allocator\n");
}

void test_arena_allocator(void) {
    printf("[UNIT TEST] Testing Arena Allocator...\n");

    beam_allocator_i alloc = beam_allocator_create_arena(1024);
    assert(alloc.alloc != NULL);
    assert(alloc.ctx != NULL);

    char* str1 = (char*)alloc.alloc(alloc.ctx, 64);
    assert(str1 != NULL);
    strcpy(str1, "Hello BEAM Modular Arena!");

    char* str2 = (char*)alloc.alloc(alloc.ctx, 128);
    assert(str2 != NULL);
    (void)str2;

    beam_memory_stats_t stats = beam_allocator_get_stats(&alloc);
    assert(stats.active_allocations == 2);
    assert(stats.total_allocated_bytes >= 192);
    (void)stats;

    /* Test overflow condition */
    void* huge_ptr = alloc.alloc(alloc.ctx, 2048);
    assert(huge_ptr == NULL); /* Should fail safely without crashing */
    (void)huge_ptr;

    beam_allocator_destroy_arena(&alloc);

    printf("[PASSED] test_arena_allocator\n");
}

int main(void) {
    printf("=========================================\n");
    printf(" RUNNING ISOLATED MODULE TEST: MEMORY    \n");
    printf("=========================================\n");
    test_system_allocator();
    test_arena_allocator();
    return 0;
}
