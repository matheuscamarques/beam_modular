# A/B Benchmarks — original BEAM vs rewrite (beam_modular)

Controlled comparison between the **original BEAM implementation** (Erlang/OTP 30,
built from `otp_src/`) and the **modular rewrite** (`src/`, modular monolith with
opaque pointers and DI via vtables).

## Overview

```
Side A (original)  -> Erlang drivers in benchmarks/drivers/a/*.erl, run on the
                      `erl`/`erlc` built from otp_src/
Side B (rewrite)   -> C drivers in benchmarks/drivers/b/bench_*.c, compiled by
                      benchmarks/drivers/b/Makefile from the src/ module
                      sources (self-contained archive libbench_ab.a, no
                      dependency on the root CMakeLists)
Harness            -> benchmarks/harness/run_ab.py
```

## How to run

```bash
# 1. Build the reference OTP 30 (in-source in otp_src/, gitignored)
cd otp_src && ./configure --without-javac --without-wx --without-odbc \
    --without-debugger --without-et --disable-dynamic-ssl-lib && make -j8
#    NOTE: lib/observer fails to build ("behaviour wx_object undefined") because
#    wx is disabled; that is harmless for erl/erlc. bin/erl, bin/erlc are
#    produced anyway by the runtime fixup stage. No-op with `-k` if desired.

# 2. Build the B drivers (self-contained makefile, outputs to benchmarks/work/bench)
make -C benchmarks/drivers/b
#    Clean rebuild: make -C benchmarks/drivers/b clean && make -C benchmarks/drivers/b

# 3. Run one workload (e.g. ets) with 7 runs + 1 warmup, pin CPU 3
python3 benchmarks/harness/run_ab.py --workload ets --runs 7 --warmup 1 --cpu 3

# 4. Run everything
python3 benchmarks/harness/run_ab.py --all
```

Reports: `benchmarks/reports/report_<timestamp>.json|md` (gitignored).
Scratch (Erlang .beam) + B binaries: `benchmarks/work/` (gitignored).

## Output protocol (identical on both sides)

```
RESULT <line>        # canonical sequence, repeated in order; PARITY = hard gate
FINGERPRINT <hex64>  # FNV-1a 64 over the concatenated lines + "\n"
TIME_US <int>        # driver-internal time (secondary)
OPS <int>            # semantic operations
METRIC k=v           # extra metrics (do not participate in parity)
```

Parity requires the `RESULT` sequence to be **byte-for-byte identical** across all
runs on both sides. Performance = median wall-time per side (median + MAD;
alternating ABBA runs; warmup discarded; `taskset` for CPU pinning).

## Workloads v1 (per-module micro-benchmarks)

| Workload | Side A (Erlang) | Side B (C) | RESULT (parity) |
|---|---|---|---|
| `loader` | `beam_lib:chunks(F, [atoms])` | `beam_file_parse` on the same `.beam` | module + atoms in file order |
| `atom` | `list_to_atom` + `atom_to_binary` | `beam_atom_put`/`beam_atom_find` | names sorted byte-for-byte |
| `ets` | `ets:new/insert/lookup/delete` | `beam_ets_*` | final count + found_all + found_rest |
| `runqueue` | spawn N procs with mixed priorities | `beam_run_queue_*` | enqueued + dequeued |
| `alloc` | `binary:copy` with size pattern | `beam_allocator_*` | ops + total_used (arithmetic) |
| `msg` | 2 processes send/receive N msgs | `beam_mailbox_*` | sent + received |
| `emu_loop` | tail recursion `loop(N-1, Acc+1)` | `beam_emu_execute_code` interpreter (N inline ADDs) | final value = N |

### Proxies and caveats (important)

- **Different semantics**: side A runs on a full Erlang VM (scheduler,
  copying GC, interpreter); side B calls the C API directly. Absolute time
  deltas are **not comparable** as "the same thing". The comparison validates:
  1. **parity** (hard gate — identical behavior), and
  2. **trend/regression** of each side's own metrics (baseline
     versioned in reports/).
- `runqueue` and `msg` are **proxies** (the Erlang scheduler does not expose
  enqueue/dequeue; the proxy is spawn/throughput and send/receive).
- `atom` depends on the in-flight atom table refactor (see "Pending items").
- `emu_loop` only measures MOVE/ADD opcodes (current interpreter subset).
- `TIME_US` of the Erlang driver includes VM boot; harness wall-time includes
  that overhead on both sides (documented systematic bias; workloads run for
  seconds, overhead is ~100ms).
- Parity is deterministic: fixed seeds/patterns, canonical ordering before
  hashing. No pointers/addresses in fingerprints.

## Pending items / detected blockers

1. **RESOLVED**: the atom table refactor to `beam_atom_intern`/`beam_atom_lookup`
   (Eterm-based) is complete in `beam_global.h` + `atom_table.c`; node table
   (`beam_node_table_*`) is implemented and unit tested. All unit tests build
   and pass (`ctest`: 17/17).
2. **Loader vs real .beam format**: the rewrite's parser reads `AtU8` with
   atom length in 1 byte; the real OTP format uses u16 (verify against a real
   `.beam` produced by erlc and fix if needed).
3. Disk: in-source otp_src build consumes ~2-3GB (monitor).

## Phase 2 (out of scope for v1)

- Expand the opcode subset and compare execution of whole `.beam` files (Code
  chunk).
- CI (GitHub Actions) with report artifacts.