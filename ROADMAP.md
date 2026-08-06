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
| L0 | Foundation/architecture (DI, opacity, vtable) | 5% | ~95% skeleton | `build` + 17 ctest |
| L1 | Loader `.beam` + Code chunk | 12% | ~10% (AtU8 only) | AtU8 u16 + instr decode |
| L2 | Term system (10 types) | 15% | ~15% (int/atom/nil/list/tuple/pid) | float, big, refc binary, map, fun… |
| L3 | Interpreter / opcodes | 22% | ~2% (13/680) | `emu_loop` parity, X/Y reg & CP stack |
| L4 | Generational copying GC | 10% | ~0% (stub) | `test_gc` real asserts + ProcBin refc |
| L5 | Scheduler / multi-scheduler | 12% | ~25% concept | preemption, N schedulers, timer wheel |
| L6 | BIFs + guard BIFs | 8% | ~1% (3/~600) | `test_bif` ratios A/B |
| L7 | Complete ETS | 5% | ~12% | set/bag/ordered + protection |
| L8 | Real NIF (`.so`, resources) | 5% | ~5% | dlopen + ABI callbacks |
| L9 | Distribution + ETF + real I/O | 3% | ~0% | EPMD handshake, ETF, epoll |

## Current completion: **~10%** (foundation verified)

---

## Milestones (10% each)

### 10% — Verified foundation *(≈ reached)*

- [x] 8 subsystems wired via `beam_vm_create` (DI/vtables)
- [x] 13 opcodes; 17/17 unit+int tests; 7 A/B workloads defined
- [x] Fix declared-but-unimplemented prototypes (`beam_code_server_module_count`, etc.)
- [x] Remove orphan duplicated legacy module `src/atom/`, `tests/test_atom.c`, `include/beam_atom.h`, `include/beam_allocator.h`, `tests/mocks/mock_allocator.*`
- [x] Define `ETERM_NIL` / `beam_is_list` / `beam_list_head` / `beam_list_tail` in modern C23 standard
- [x] Standardize CMake on C23 (`set(CMAKE_C_STANDARD 23)`) with `[[nodiscard]]` & `static_assert`
- **Gate:** `make build` + `ctest` 17/17 PASS

### 20% — Real loader + core terms

- Chunks: `AtU8` with **u16** lengths (fix 1-byte), plus `LitT`, `ExpT`, `ImpT`, `StrT`
- Implement `beam_module_load_from_memory` / `beam_module_destroy`
- Terms: **float (boxed)**, **big int**, **ref**, **port**, **refc binaries (HeapBin vs ProcBin)**; full `beam_is_*` set
- **Gate:** `test_beam_loader` against a real `erlc` `.beam`; `test_terms` clean

### 30% — Useful interpreter (real subset & stack frame)

- [ ] Decode Code chunk → `beam_instruction_t`
- [ ] Opcodes: 13 existing + `test`, `is_*` guards, `get_list`, `select_val`, `allocate`, `deallocate`
- [ ] Stack Frame & Registers: CP (Continuation Pointer), X-registers (`x[0]..x[1023]`), Y-registers (stack slots)
- [ ] Run `sample_module.beam` (real file), not just inline instructions
- **Gate:** `emu_loop` + `loader` parity; opcount ≥ 30

### 40% — Copying GC + process heaps

- [ ] Replace `beam_gc_collect_process` stub: from→to-space copy, roots = X/Y regs/stack/CP
- [ ] Off-Heap Ref-Counted Binaries (`ProcBin`) memory release & destruction
- [ ] Heap trim; young/old start; tuples/messages with heap traces
- [ ] **Gate:** `test_gc` asserts object movement and ProcBin refcounts (not just `BEAM_OK`)

### 50% — Multi-scheduler + fair preemption + Timer Wheel

- [ ] Thread pool scheduler (N schedulers), per-scheduler queues
- [ ] Real priorities MAX/HIGH/NORMAL/LOW; process migration
- [ ] Timer Wheel / Min-Heap timers for `receive after` timeout states
- [ ] **Gate:** expanded `test_scheduler_loop`; N concurrent procs race-free (helgrind)

### 60% — BIFs + pattern receive

- [ ] ~300 BIFs: name→function BIF table with real arity check; guard BIFs for new types
- [ ] Receive with **pattern matching** (not free dequeue); timeout support
- [ ] **Gate:** `+`, `-`, `*`, `/`, `==`, `=:=`, `>`, `<` parity

### 70% — ETS + real code server

- [ ] ETS: set/ordered_set/bag/duplicate_bag; read_concurrency protection; owner/heir
- [ ] Code server: exports/imports tables, purge/reload, `module_info`-like
- [ ] **Gate:** `ets` workload parity with new subset

### 80% — Real NIF (`.so`)

- [ ] `enif_open_resource_type`, resources + GC, binaries
- [ ] `dlopen` `.so`, OTP ABI callbacks; drop custom `ErlNifEnv` API
- [ ] **Gate:** load a real NIF `.so` and call from interpreter

### 90% — Distribution + ETF + EPMD

- [ ] `term_to_binary` / `binary_to_term` (External Term Format)
- [ ] EPMD client lookup (port 4369) & Cookie challenge/response digest handshake
- [ ] Socket handshake + node protocol; remote `!`
- [ ] **Gate:** 2 VMs talk over socket; `test_dist_node` uses real handshake (no array stub)

### 100% — OTP-equivalent core VM

- [ ] Real I/O (epoll/fd), ports, async
- [ ] `erlang:halt`/dump, VM clock, tidy shutdown
- [ ] `run_ab.py --all` all workloads parity PASS
- [ ] Zero `TODO`/stubs in `src/`
- [ ] CI enforces gates on every PR

## How to move the needle

1. Pick the next incomplete milestone (lowest %).
2. Each PR updates this file's checkboxes and the **§ Summary** line with the new %.
3. CI (`make build` && `make test` && `run_ab.py --all`) is the gatekeeper; a layer only counts when green.

---

*Status: 2026-08-06 — at ~10%. Fully updated and verified.*