# BEAM Modular Monolith

A modular rewrite of the Erlang/OTP virtual machine (BEAM) in pure C11, refactored from the legacy code in `otp_src/erts/emulator/beam/`. The goal is to rebuild the essential VM components as a strict **Modular Monolith** with full encapsulation via **Opaque Pointers**, **Dependency Injection (vtables)** and **strict module isolation** — keeping the original VM as the reference (side A) for parity and performance validation (side B).

## Objective

Prove that the BEAM core (loader, code server, scheduler, run queue, processes, messaging, ETS, atom table, allocator, GC, NIFs, interpreter) can be reimplemented with zero global state, zero direct cross-module coupling and a clean build under `-Wall -Wextra -Wpedantic -Wshadow -Werror`, while keeping **byte-for-byte output parity** with the original OTP.

## Layout

```
beam_modular/
├── include/           # PUBLIC INTERFACES (.h) — prototypes and opaque pointers only
│   └── beam_*.h       # NEVER contains struct definitions (except DI vtables)
├── src/               # PRIVATE IMPLEMENTATION (.c / _internal.h)
│   ├── core/          # Entry point (beam_main.c) and orchestrator (beam_vm.c)
│   ├── atom/          # Isolated atom table
│   ├── memory/        # Allocator (beam_alloc) and GC (beam_gc)
│   ├── messaging/     # Mailboxes and message delivery
│   ├── scheduler/     # Processes, priority run queue, scheduler loop
│   ├── emulator/      # Interpreter (beam_emu), BIFs, .beam loader, code server
│   ├── global/        # Atom table, ETS, distributed nodes
│   └── io/            # Event poller and NIF environment (ErlNifEnv)
├── tests/             # Isolated unit tests + integration + mocks
│   ├── mocks/         # Dependency mocks (mock_memory, mock_allocator)
│   ├── unit/          # 16 per-module test suites
│   └── integration/   # test_full_vm (full VM wired via DI)
├── benchmarks/        # A/B harness: original OTP (A) vs rewrite (B)
├── otp_src/           # OTP legacy source mirror (reference only)
├── CMakeLists.txt     # Strict C11 build
├── Makefile           # Shortcuts: build / test / train-pgo / clean
└── AGENT.md           # Engineering rules and refactoring protocol
```

## Engineering principles

1. **Zero global state** — no `static`/`extern` shared across files; all state lives in context structs (`beam_context_t`, `beam_process_t`, ...) passed by pointer.
2. **Opaque pointers** — `include/beam_*.h` only exposes `typedef struct beam_<mod> beam_<mod>_t;` and prototypes. Concrete definitions live in `src/<mod>/..._internal.h`.
3. **Restrictive encapsulation** — field access outside the owning `.c` is forbidden; use accessors (`beam_process_get_state`, `beam_process_consume_reductions`).
4. **Dependency Injection (vtables)** — direct calls across modules are prohibited; interfaces such as `beam_allocator_i` are injected at creation/init time.
5. **Build integrity** — any new file updates `CMakeLists.txt` immediately; compiler warnings are treated as errors.

## Modules

| Subsystem | Files | Responsibility |
|---|---|---|
| `core` | `beam_vm.c`, `beam_main.c` | VM orchestration, wiring all dependencies via vtables |
| `atom` | `beam_atom.c` | Isolated atom table (interning) |
| `memory` | `beam_alloc.c`, `beam_gc.c` | DI allocator and process-heap garbage collector |
| `messaging` | `erl_message.c` | Mailboxes and process-to-process delivery |
| `scheduler` | `erl_process.c`, `run_queue.c`, `beam_scheduler_loop.c` | Process life-cycle, priority run queue, reduction-based preemption |
| `emulator` | `beam_emu.c`, `beam_bif.c`, `beam_load.c`, `beam_code_server.c` | Bytecode interpreter, BIFs, `.beam` (AtU8) parsing and module registry |
| `global` | `atom_table.c`, `ets.c`, `beam_node.c` | Atom table, ETS and node identity |
| `io` | `sys_poll.c`, `erl_nif.c` | Event poller with fd registration and opaque NIF environment |

## Build

Requirements: CMake ≥ 3.14, C11 compiler (GCC/Clang), Python 3 for the A/B harness, and an OTP build in `otp_src/` for side A.

```bash
make build    # cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build
make test     # ctest --test-dir build --output-on-failure
```

Strict flags: `-Wall -Wextra -Wpedantic -Wshadow -Werror`. Optional LTO via `-DENABLE_LTO=ON`.

## Tests

`ctest` runs 17 executables: 16 unit suites (one per module, with mocks) and 1 full-VM integration test. Examples: `test_atom_table`, `test_process`, `test_run_queue`, `test_gc`, `test_emu`, `test_bif`, `test_loader`, `test_code_server`, `test_ets`, `test_messaging`, `test_io`, `test_nif`, `test_memory`, `test_terms`, `test_dist_node`, `test_scheduler_loop`.

## A/B benchmarks

The `benchmarks/harness/run_ab.py` harness compares both sides:

- **Side A** — the original Erlang/OTP VM (Erlang drivers in `benchmarks/drivers/a/`, run with `erl` from `otp_src/`).
- **Side B** — the modular rewrite (C drivers in `benchmarks/drivers/b/`, built with `-DENABLE_BENCH=ON`).

**Parity is the hard gate**: the `RESULT` sequence of all runs and sides must be identical byte-for-byte. Performance uses median wall-time with alternating ABBA ordering, warmup and CPU pinning (`taskset`). Workloads: `loader`, `atom`, `ets`, `runqueue`, `alloc`, `msg`, `emu_loop` (catalog in `benchmarks/workloads.json`). Markdown/JSON reports land in `benchmarks/reports/`.

```bash
cmake -B build -DENABLE_BENCH=ON && cmake --build build
python3 benchmarks/harness/run_ab.py --all            # run all workloads
python3 benchmarks/harness/run_ab.py --workload atom  # a single workload
```

## Current status

- All subsystems implemented and wired through `beam_vm_create` (full DI).
- 16 modules with isolated unit tests plus integration passing.
- Functional A/B harness with byte-for-byte parity as the validation criterion.
- In flight: interpreter with new opcodes (`MATCH_TUPLE`, `GET_TUPLE_ELEMENT`, `SEND`, `RECEIVE`), scheduler loop with reduction-based preemption.