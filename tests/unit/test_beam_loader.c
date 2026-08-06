#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "beam_core.h"
#include "mock_memory.h"

void test_beam_binary_parser(void) {
    printf("[UNIT TEST] Testing BEAM Binary File Loader & Chunk Parser...\n");

    mock_memory_stats_t stats = {0};
    beam_allocator_i alloc = mock_memory_create(&stats);

    /* Construct Mock Valid BEAM File Buffer:
     * 0..3: "FOR1"
     * 4..7: total_len (u32 big endian) = 36
     * 8..11: "BEAM"
     * --- AtU8 Chunk ---
     * 12..15: "AtU8"
     * 16..19: chunk_len = 20
     * 20..23: num_atoms = 2
     * 24: len = 9, string = "my_module"
     * 34: len = 5, string = "hello"
     * 40: 0 (padding)
     */
    uint8_t mock_beam[] = {
        'F', 'O', 'R', '1',
        0x00, 0x00, 0x00, 0x24, /* 36 bytes payload */
        'B', 'E', 'A', 'M',
        /* AtU8 Chunk */
        'A', 't', 'U', '8',
        0x00, 0x00, 0x00, 0x14, /* 20 bytes */
        0x00, 0x00, 0x00, 0x02, /* 2 atoms */
        9, 'm', 'y', '_', 'm', 'o', 'd', 'u', 'l', 'e',
        5, 'h', 'e', 'l', 'l', 'o'
    };

    beam_file_t* beam = beam_file_parse(mock_beam, sizeof(mock_beam), &alloc);
    assert(beam != NULL);

    const char* mod_name = beam_file_get_module_name(beam);
    assert(mod_name != NULL);
    assert(strcmp(mod_name, "my_module") == 0);

    assert(beam_file_get_atom_count(beam) == 2);
    assert(strcmp(beam_file_get_atom(beam, 0), "my_module") == 0);
    assert(strcmp(beam_file_get_atom(beam, 1), "hello") == 0);

    beam_file_destroy(beam);

    assert(stats.alloc_count > 0);
    assert(stats.free_count == stats.alloc_count);
    printf("  [RESULT] Module '%s' parsed cleanly with 2 atoms!\n", mod_name);
    printf("  [PASSED] test_beam_binary_parser\n");
}

int main(void) {
    printf("=========================================\n");
    printf(" RUNNING ISOLATED MODULE TEST: LOADER   \n");
    printf("=========================================\n");
    test_beam_binary_parser();
    return 0;
}
