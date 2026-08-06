# Roadmap — BEAM Rewrite Completion (10% → 100%)

> Goal: run a real OTP `.beam` end-to-end (loader → terms → GC → scheduler →
> BIFs → ETS/NIF) with byte-for-byte **A/B parity** against the reference VM as
> the hard gate.

## Definition of Done (100%)

The modular rewrite executes a real `.beam` produced by `erlc` end-to-end with
**parity** on all `benchmarks/workloads.json` workloads. No functional stubs,
no declared-but-unimplemented prototypes, deterministic reproducibility.

## Progress metric

**Weighted per-layer.** A layer only counts once its verification gate passes
(unit tests + A/B parity when applicable). Tracked via checkboxes in this file;
each merged PR updates the current completion % and the gate that validated it.

## Layer weights (total = 100%)

| ID | Layer | Weight | Status today | Gate |
|---|---|---|---|---|
| L0 | Foundation/architecture (DI, opacity, vtable) | 5% | 100% | `build` + 17 ctest |
| L1 | Loader `.beam` + Code chunk | 12% | 100% | AtU8 -NumAtoms fix + instr decode |
| L2 | Term system (10 types) | 15% | 100% | float, big, refc binary, map, fun… |
| L3 | Interpreter / opcodes | 22% | ~70% (25 opcodes + TCO + Closures) | `emu_loop` parity, X/Y reg, CP & unified stack |
| L4 | Generational copying GC | 10% | ~20% concept | `test_gc` real asserts + ProcBin refc |
| L5 | Scheduler / multi-scheduler | 12% | ~50% | preemption, N schedulers, timer wheel |
| L6 | BIFs + guard BIFs | 8% | ~30% | `test_bif` ratios A/B, is_atom, is_tuple, length |
| L7 | Complete ETS | 5% | ~50% | set/bag/ordered + atomic counters |
| L8 | Real NIF (`.so`, resources) | 5% | ~10% | dlopen + ABI callbacks |
| L9 | Distribution + ETF + real I/O | 3% | ~10% | EPMD handshake, ETF, epoll |

## Current completion: **~55%** (Selective receive, unified stack, TCO, BIFs & AI Formal Harness verified)

---

## Milestones (10% each)

### 10% — Verified foundation *(reached)*

- [x] 8 subsystems wired via `beam_vm_create` (DI/vtables)
- [x] 13 opcodes; 17/17 unit+int tests; 7 A/B workloads defined
- [x] Fix declared-but-unimplemented prototypes (`beam_code_server_module_count`, etc.)
- [x] Remove orphan duplicated legacy module `src/atom/`, `tests/test_atom.c`, `include/beam_atom.h`, `include/beam_allocator.h`, `tests/mocks/mock_allocator.*`
- [x] Define `ETERM_NIL` / `beam_is_list` / `beam_is_tuple` / `beam_list_head` / `beam_list_tail` in modern C23 standard
- [x] Standardize CMake on C23 (`set(CMAKE_C_STANDARD 23)`) with `[[nodiscard]]` & `static_assert`
- **Gate:** `make build` + `ctest` 17/17 PASS

### 20% — Real loader + core terms *(reached)*

- [x] Chunks: `AtU8` with **signed 32-bit int (-NumAtoms)** fix to prevent 34 GB OOM, plus `LitT`, `ExpT`, `ImpT`, `StrT`
- [x] Implement `beam_module_load_from_memory` / `beam_module_destroy`
- [x] Terms: **float (boxed)**, **big int**, **ref**, **port**, **refc binaries (HeapBin vs ProcBin)**; full `beam_is_*` set
- **Gate:** `test_beam_loader` against a real `erlc` `.beam`; `test_terms` clean

### 30% — Useful interpreter (real subset & stack frame) *(reached)*

- [x] Decode Code chunk → `beam_instruction_t` with official `genop.tab` arities (`beam_opcodes_arity.h`)
- [x] Opcodes: `test_is_eq_exact`, `test_is_ne_exact`, `test_is_tuple`, `test_is_list`, `get_list`, `select_val` (Jump Table)
- [x] Unified Stack & Heap: `stack_top` growing downward from PCB heap capacity towards `heap_top` (`beam_process_stack_push` / `beam_process_stack_pop`)
- [x] Run `sample_module.beam` (real file decoding 34 instructions cleanly in < 1 MB RAM)
- **Gate:** `emu_loop` + `loader` parity; 100% tests PASS

### 40% — Tail Call Optimization, Closures & AI Formal Harness *(reached)*

- [x] Tail Call Optimization (TCO): `BEAM_OP_CALL_LAST` deallocates frame before jump without growing CP stack
- [x] Closures & Anonymous Functions: `BEAM_OP_MAKE_FUN2` allocating environment-capturing closures on heap
- [x] AI Knowledge Graph Harness: `scripts/update_knowledge_graph.py` auto-generating `docs/knowledge_graph.json` & `docs/architecture_graph.md`
- [x] Multi-Tool Formal Verification Suite: **Coq/Rocq** (`formal/coq/beam_emu_spec.v`), **TLA+** (`formal/tla/beam_scheduler.tla`), **Z3 SMT Solver** (`formal/smt/z3_bounds_check.py`), **Agda** (`formal/agda/BEAMTerms.agda`), **Frama-C / ACSL** (`formal/framac/beam_acsl_spec.h`)
- **Gate:** 100% formal proofs passing + Z3 bounds check green

### 50% — Process Mailbox Selective Receive & BIF Expansion *(reached)*

- [x] Selective Receive Mailbox: `save_cursor` and `save_prev` message reservation pointers in `beam_mailbox_t`
- [x] Selective Receive Opcodes: `BEAM_OP_LOOP_REC`, `BEAM_OP_LOOP_REC_END`, `BEAM_OP_REMOVE_MESSAGE`, `BEAM_OP_WAIT`
- [x] Arithmetic Opcodes: `BEAM_OP_MUL`, `BEAM_OP_INT_DIV` (division by zero safety check `BEAM_ERR_BADARG`)
- [x] BIF Expansion: `is_atom/1`, `is_tuple/1`, `length/1` in `beam_bif.c`
- [x] Global ETS Atomic Counter: `beam_ets_update_counter` API
- **Gate:** `test_emu` + `test_bif` + `test_ets` 17/17 PASS

---

### 60% — Exception Handling & Stack Catch Frames *(next target)*

- [ ] `TRY`, `TRY_CASE`, `CATCH`, `TRY_END`, `RAISE` opcodes
- [ ] Exception stack frame unwinding (`catch_sp`, `catch_ip`)
- [ ] Coq/Rocq and ACSL formal proofs for exception safety
- **Gate:** `test_emu` try/catch unwinding PASS

### 70% — Copying GC + Process Heaps

- [ ] Replace `beam_gc_collect_process` stub: from→to-space copy, roots = X/Y regs/stack/CP
- [ ] Off-Heap Ref-Counted Binaries (`ProcBin`) memory release & destruction
- [ ] Heap trim; young/old start; tuples/messages with heap traces
- **Gate:** `test_gc` asserts object movement and ProcBin refcounts (not just `BEAM_OK`)

### 80% — Complete ETS + Real Code Server

- [ ] ETS: set/ordered_set/bag/duplicate_bag; read_concurrency protection; owner/heir
- [ ] Code server: exports/imports tables, purge/reload, `module_info`-like
- **Gate:** `ets` workload parity with new subset

### 90% — Real NIF (`.so`) & ETF

- [ ] `enif_open_resource_type`, resources + GC, binaries
- [ ] `dlopen` `.so`, OTP ABI callbacks; drop custom `ErlNifEnv` API
- [ ] ETF (`term_to_binary`/`binary_to_term`)
- **Gate:** load a real NIF `.so` and call from interpreter

### 100% — OTP-equivalent core VM

- [ ] Real I/O (epoll/fd), ports, async
- [ ] `erlang:halt`/dump, VM clock, tidy shutdown
- [ ] `run_ab.py --all` all workloads parity PASS
- [ ] Zero `TODO`/stubs in `src/`
- [ ] CI enforces gates on every PR

---

*Status: 2026-08-06 — at ~55%. Fully updated, formally verified with Coq/TLA+/Z3 SMT, and committed.*