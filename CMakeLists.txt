cmake_minimum_required(VERSION 3.14)
project(beam_modular C)

# Set C Standard to C23 (Modern C23 ISO Standard)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Security and strict warning flags
add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Werror)

# LTO (Link-Time Optimization) support (disabled by default for test stability)
option(ENABLE_LTO "Enable Link-Time Optimization" OFF)
if(ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
    if(ipo_supported)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endif()

# A/B benchmark drivers (sides B). Enabled via -DENABLE_BENCH=ON.
option(ENABLE_BENCH "Build A/B benchmark drivers" OFF)
if(ENABLE_BENCH)
    add_executable(bench_loader benchmarks/drivers/b/bench_loader.c)
    target_include_directories(bench_loader PRIVATE src/emulator)
    target_link_libraries(bench_loader PRIVATE beam_emulator_mod beam_memory_mod)

    add_executable(bench_atom benchmarks/drivers/b/bench_atom.c)
    target_link_libraries(bench_atom PRIVATE beam_global_mod beam_memory_mod)

    add_executable(bench_ets benchmarks/drivers/b/bench_ets.c)
    target_link_libraries(bench_ets PRIVATE beam_global_mod beam_memory_mod)

    add_executable(bench_runqueue benchmarks/drivers/b/bench_runqueue.c)
    target_link_libraries(bench_runqueue PRIVATE beam_scheduler_mod beam_messaging_mod beam_memory_mod)

    add_executable(bench_alloc benchmarks/drivers/b/bench_alloc.c)
    target_link_libraries(bench_alloc PRIVATE beam_memory_mod)

    add_executable(bench_msg benchmarks/drivers/b/bench_msg.c)
    target_link_libraries(bench_msg PRIVATE beam_messaging_mod beam_memory_mod)

    add_executable(bench_emu benchmarks/drivers/b/bench_emu.c)
    target_include_directories(bench_emu PRIVATE src/emulator)
    target_link_libraries(bench_emu PRIVATE beam_emulator_mod beam_scheduler_mod beam_messaging_mod beam_memory_mod)
endif()

# Include Public Headers
include_directories(include)

# Module Libraries
add_library(beam_memory_mod STATIC src/memory/beam_alloc.c src/memory/beam_gc.c)
target_include_directories(beam_memory_mod PRIVATE src/memory)

add_library(beam_messaging_mod STATIC src/messaging/erl_message.c)
target_include_directories(beam_messaging_mod PRIVATE src/messaging)

add_library(beam_scheduler_mod STATIC src/scheduler/erl_process.c src/scheduler/run_queue.c src/scheduler/beam_scheduler_loop.c)
target_include_directories(beam_scheduler_mod PRIVATE src/scheduler src/emulator)
target_link_libraries(beam_scheduler_mod PRIVATE beam_messaging_mod)

add_library(beam_emulator_mod STATIC src/emulator/beam_emu.c src/emulator/beam_decode.c src/emulator/beam_bif.c src/emulator/beam_load.c src/emulator/beam_code_server.c)
target_include_directories(beam_emulator_mod PRIVATE src/emulator)

add_library(beam_global_mod STATIC src/global/atom_table.c src/global/ets.c src/global/dist_node.c)
target_include_directories(beam_global_mod PRIVATE src/global)

add_library(beam_io_mod STATIC src/io/sys_poll.c src/io/erl_nif.c)
target_include_directories(beam_io_mod PRIVATE src/io)

add_library(beam_core_mod STATIC src/core/beam_vm.c)
target_include_directories(beam_core_mod PRIVATE src/core)
target_link_libraries(beam_core_mod PRIVATE beam_memory_mod beam_scheduler_mod beam_emulator_mod beam_messaging_mod beam_global_mod beam_io_mod)

add_library(beam_mocks STATIC tests/mocks/mock_memory.c)
target_include_directories(beam_mocks PUBLIC tests/mocks)

# Core Executable
add_executable(beam_core_bin src/core/beam_main.c)
target_link_libraries(beam_core_bin PRIVATE beam_core_mod)

# Unit & Integration Tests
enable_testing()

add_executable(test_dist_node tests/unit/test_dist_node.c)
target_link_libraries(test_dist_node PRIVATE beam_global_mod beam_mocks)
add_test(NAME test_dist_node COMMAND test_dist_node)

add_executable(test_full_vm tests/integration/test_full_vm.c)
target_link_libraries(test_full_vm PRIVATE beam_core_mod beam_mocks)
add_test(NAME test_full_vm COMMAND test_full_vm)

add_executable(test_scheduler_loop tests/unit/test_scheduler_loop.c)
target_include_directories(test_scheduler_loop PRIVATE src/emulator)
target_link_libraries(test_scheduler_loop PRIVATE beam_scheduler_mod beam_emulator_mod beam_messaging_mod beam_mocks)
add_test(NAME test_scheduler_loop COMMAND test_scheduler_loop)

add_executable(test_code_server tests/unit/test_code_server.c)
target_include_directories(test_code_server PRIVATE src/emulator)
target_link_libraries(test_code_server PRIVATE beam_emulator_mod beam_mocks)
add_test(NAME test_code_server COMMAND test_code_server)

add_executable(test_nif tests/unit/test_nif.c)
target_link_libraries(test_nif PRIVATE beam_io_mod beam_scheduler_mod beam_messaging_mod beam_mocks)
add_test(NAME test_nif COMMAND test_nif)

add_executable(test_run_queue tests/unit/test_run_queue.c)
target_link_libraries(test_run_queue PRIVATE beam_scheduler_mod beam_messaging_mod beam_mocks)
add_test(NAME test_run_queue COMMAND test_run_queue)

add_executable(test_io tests/unit/test_io.c)
target_link_libraries(test_io PRIVATE beam_io_mod beam_mocks)
add_test(NAME test_io COMMAND test_io)

add_executable(test_beam_loader tests/unit/test_beam_loader.c)
target_include_directories(test_beam_loader PRIVATE src/emulator)
target_link_libraries(test_beam_loader PRIVATE beam_emulator_mod beam_mocks)
add_test(NAME test_beam_loader COMMAND test_beam_loader)

add_executable(test_terms tests/unit/test_terms.c)
target_link_libraries(test_terms PRIVATE beam_scheduler_mod beam_messaging_mod beam_mocks)
add_test(NAME test_terms COMMAND test_terms)

add_executable(test_memory tests/unit/test_memory.c)
target_link_libraries(test_memory PRIVATE beam_memory_mod)
add_test(NAME test_memory COMMAND test_memory)

add_executable(test_gc tests/unit/test_gc.c)
target_link_libraries(test_gc PRIVATE beam_memory_mod beam_scheduler_mod beam_messaging_mod beam_mocks)
add_test(NAME test_gc COMMAND test_gc)

add_executable(test_process tests/unit/test_process.c)
target_link_libraries(test_process PRIVATE beam_scheduler_mod beam_messaging_mod beam_mocks)
add_test(NAME test_process COMMAND test_process)

add_executable(test_emu tests/unit/test_emu.c)
target_include_directories(test_emu PRIVATE src/emulator)
target_link_libraries(test_emu PRIVATE beam_emulator_mod beam_scheduler_mod beam_messaging_mod beam_mocks)
add_test(NAME test_emu COMMAND test_emu)

add_executable(test_bif tests/unit/test_bif.c)
target_include_directories(test_bif PRIVATE src/emulator)
target_link_libraries(test_bif PRIVATE beam_emulator_mod beam_scheduler_mod beam_messaging_mod beam_mocks)
add_test(NAME test_bif COMMAND test_bif)

add_executable(test_messaging tests/unit/test_messaging.c)
target_link_libraries(test_messaging PRIVATE beam_messaging_mod beam_scheduler_mod beam_global_mod beam_mocks)
add_test(NAME test_messaging COMMAND test_messaging)

add_executable(test_ets tests/unit/test_ets.c)
target_link_libraries(test_ets PRIVATE beam_global_mod beam_mocks)
add_test(NAME test_ets COMMAND test_ets)

add_executable(test_atom_table tests/unit/test_atom_table.c)
target_link_libraries(test_atom_table PRIVATE beam_global_mod beam_mocks)
add_test(NAME test_atom_table COMMAND test_atom_table)
