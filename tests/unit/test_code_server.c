#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "beam_core.h"
#include "beam_code_server_internal.h"
#include "mock_memory.h"

void test_code_server_registry(void) {
    printf("[UNIT TEST] Testing Code Server Module Registry...\n");

    mock_memory_stats_t stats = {0};
    beam_allocator_i alloc = mock_memory_create(&stats);

    beam_code_server_t* cs = beam_code_server_create(&alloc);
    assert(cs != NULL);
    assert(beam_code_server_module_count(cs) == 0);

    /* Construct mock BEAM binary file buffer with Atom chunk containing math_module */
    uint8_t buffer[12 + 8 + 30];
    memset(buffer, 0, sizeof(buffer));

    /* RIFF Header: FOR1 ... BEAM */
    memcpy(&buffer[0], "FOR1", 4);
    uint32_t total_len = 8 + 30;
    buffer[4] = (total_len >> 24) & 0xFF;
    buffer[5] = (total_len >> 16) & 0xFF;
    buffer[6] = (total_len >> 8) & 0xFF;
    buffer[7] = total_len & 0xFF;
    memcpy(&buffer[8], "BEAM", 4);

    /* Atom Chunk: Atom ... len=30 ... count=1 ... "math_module" */
    memcpy(&buffer[12], "Atom", 4);
    uint32_t chunk_len = 22;
    buffer[16] = (chunk_len >> 24) & 0xFF;
    buffer[17] = (chunk_len >> 16) & 0xFF;
    buffer[18] = (chunk_len >> 8) & 0xFF;
    buffer[19] = chunk_len & 0xFF;
    buffer[23] = 1; /* atom count = 1 */
    buffer[24] = 11; /* string len = 11 */
    memcpy(&buffer[25], "math_module", 11);

    beam_file_t* file = beam_file_parse(buffer, 12 + 8 + 30, &alloc);
    assert(file != NULL);

    beam_result_t res = beam_code_server_register_module(cs, "math_module", file);
    assert(res == BEAM_OK);
    (void)res;
    assert(beam_code_server_module_count(cs) == 1);

    /* Lookup module */
    beam_file_t* found = beam_code_server_lookup_module(cs, "math_module");
    assert(found == file);
    (void)found;

    /* Lookup missing module */
    beam_file_t* not_found = beam_code_server_lookup_module(cs, "unknown_module");
    assert(not_found == NULL);
    (void)not_found;

    /* beam_code_server_destroy owns and frees all registered modules */
    beam_code_server_destroy(cs);

    assert(stats.alloc_count > 0);
    assert(stats.free_count == stats.alloc_count);
    printf("  [MEMORY] Clean memory stats! Allocs: %zu, Frees: %zu\n", stats.alloc_count, stats.free_count);
    printf("[PASSED] test_code_server_registry\n");
}

int main(void) {
    test_code_server_registry();
    return 0;
}
