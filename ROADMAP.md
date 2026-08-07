# Roadmap — BEAM Rewrite Phase 2 (SMP Multi-Threading & Distribution)

> Goal: Transform the C23 BEAM Modular Monolith into a fully concurrent **Multi-Scheduler Thread Pool (SMP)** VM with Work-Stealing, Non-blocking Epoll I/O, Distribution (EPMD), and Production-Grade Thread Safety.

## Definition of Done (Phase 2 - 100%)

The BEAM C23 VM runs multiple concurrent OS threads (one per CPU core) executing processes in parallel with work-stealing, lock-free/mutex-protected mailboxes, non-blocking I/O multiplexing (`epoll`), and distributed node clustering, passing all Helgrind/TSan race detectors and A/B benchmarks with full parity.

---

## Phase 2 Layer Breakdown & Progress Metrics

| ID | Layer / Subsystem | Weight | Current Status | Validation Gate |
|---|---|---|---|---|
| **L0** | **Thread-Safety Primitives** | 15% | **100% (Completed)** | `pthread_mutex` & `pthread_rwlock` in Mailbox, RunQueue, AtomTable (`17/17 ctest PASS`) |
| **L1** | **Multi-Scheduler Thread Pool** | 20% | **0% (Next Target)** | `thrd_create` / `pthread_create` spawning $N$ parallel scheduler loops |
| **L2** | **Work-Stealing Scheduler** | 20% | **0%** | Idle schedulers stealing processes from busy RunQueues without deadlock |
| **L3** | **Async Epoll Driver & Port I/O** | 20% | **0%** | Non-blocking socket polling (`sys_poll.c` via Linux `epoll_wait`) for Process Ports |
| **L4** | **Distributed Nodes & EPMD Handshake** | 15% | **0%** | External node clustering, ETF serialization over TCP & node handshake |
| **L5** | **Concurrency Race Audit (TSan & Helgrind)** | 10% | **0%** | Clean Valgrind Helgrind & ThreadSanitizer execution under high stress |

---

## Current Completion (Phase 2): **15%** (Thread-Safety Primitives Integrated)

---

## Milestones (20% each)

### 20% — Thread Safety Primitives & Synchronized Core *(reached: 15%)*
- [x] Process Mailbox mutex locking (`pthread_mutex_t lock`) in `src/messaging/erl_message.c`
- [x] Atom Table Read-Write Lock (`pthread_rwlock_t rwlock`) in `src/global/atom_table.c`
- [x] RunQueue mutex locking in `src/scheduler/run_queue.c`
- [x] Formal verification invariant check (`z3_bounds_check.py`) & `ctest` 17/17 PASS
- **Gate:** `ctest` 17/17 PASS + Z3 bounds check green

### 40% — Multi-Scheduler Thread Pool ($N$ Threads) *(next target)*
- [ ] Implement `beam_scheduler_pool_create(uint32_t num_threads)` in `src/scheduler/beam_scheduler_loop.c`
- [ ] Spawn $N$ OS threads executing concurrent `beam_scheduler_step` loops
- [ ] Condition variables (`pthread_cond_t`) for waking idle scheduler threads upon process enqueue
- **Gate:** Parallel execution test with $N$ worker threads concurrently executing processes

### 60% — Work-Stealing Scheduler Algorithm
- [ ] Per-scheduler local RunQueues with lock-free/spinlock steal interfaces
- [ ] Implement work-stealing heuristic when a scheduler queue becomes empty
- [ ] Preemption and lock-free reduction counter decrements
- **Gate:** Multi-core CPU utilization benchmark showing balanced thread load

### 80% — Asynchronous Epoll I/O Driver & Ports
- [ ] Integration of Linux `epoll_create1` / `epoll_wait` in `src/io/sys_poll.c`
- [ ] Process port I/O dispatching socket events directly into process mailboxes
- [ ] Async socket drivers (`gen_tcp` minimal backend)
- **Gate:** Concurrent socket ping-pong benchmark with 10,000 active connections

### 100% — Distribution, EPMD & Zero-Race Audit
- [ ] EPMD (Erlang Port Mapper Daemon) client and node handshake protocol in `src/global/dist_node.c`
- [ ] ETF (External Term Format) network packaging for cross-node process messaging
- [ ] Helgrind & ThreadSanitizer (TSan) automated CI suite with 0 race warnings
- **Gate:** 2 distributed BEAM C23 nodes exchanging messages over TCP with 0 TSan warnings

---

*Status: 2026-08-06 — Phase 2 started at 15% completion.*