#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "beam_core.h"
#include "mock_memory.h"

static void write_u32_be(uint8_t* buf, uint32_t val) {
    buf[0] = (val >> 24) & 0xFF;
    buf[1] = (val >> 16) & 0xFF;
    buf[2] = (val >> 8) & 0xFF;
    buf[3] = val & 0xFF;
}

void test_code_server_registry(void) {
    printf("[UNIT TEST] Testing Code Server Module Registry...\n");

    mock_memory_stats_t stats = {0};
    beam_allocator_i alloc = mock_memory_create(&stats);

    beam_code_server_t* cs = beam_code_server_create(&alloc);
    assert(cs != NULL);
    assert(beam_code_server_module_count(cs) == 0);

    /* Construct mock BEAM binary */
    uint8_t buffer[256];
    memset(buffer, 0, sizeof(buffer));

    memcpy(buffer, "FOR1", 4);
    write_u32_be(buffer + 4, 100);
    memcpy(buffer + 8, "BEAM", 4);

    uint8_t* chunk_ptr = buffer + 12;
    memcpy(chunk_ptr, "AtU8", 4);
    write_u32_be(chunk_ptr + 4, 30);
    write_u32_be(chunk_ptr + 8, 1);
    chunk_ptr[12] = 11;
    memcpy(chunk_ptr + 13, "math_module", 11);

    beam_file_t* file = beam_file_parse(buffer, 12 + 8 + 30, &alloc);
    assert(file != NULL);

    beam_result_t res = beam_code_server_register_module(cs, "math_module", file);
    assert(res == BEAM_OK);
    assert(beam_code_server_module_count(cs) == 1);
    (void)res;

    beam_file_t* found = beam_code_server_lookup_module(cs, "math_module");
    assert(found == file);
    (void)found;

    beam_file_t* not_found = beam_code_server_lookup_module(cs, "unknown_module");
    assert(not_found == NULL);
    (void)not_found;

    beam_code_server_destroy(cs);

    assert(stats.alloc_count > 0);
    assert(stats.free_count == stats.alloc_count);
    printf("  [RESULT] Module 'math_module' registered & found cleanly in Code Server!\n");
    printf("  [PASSED] test_code_server_registry\n");
}

int main(void) {
    printf("=========================================\n");
    printf(" RUNNING ISOLATED MODULE TEST: CODE SVR  \n");
    printf("=========================================\n");
    test_code_server_registry();
    return 0;
}
