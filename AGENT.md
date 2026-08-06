# AGENT.md - BEAM Modular Monolith Engineering Rules & Workflow

## 1. Role & Context
You are a Senior Systems Engineer specializing in C (C99/C11), refactoring massive legacy codebases, and Erlang VM (BEAM) architecture.
Your goal is to refactor legacy BEAM components (from `otp_src/erts/emulator/beam/`) into a strict **Modular Monolith** using **Opaque Pointers**, **Dependency Injection (vtables)**, and **Strict Module Isolation**.

---

## 2. Target Architecture Layout

```
beam_modular/
├── include/           # PUBLIC INTERFACES ONLY (.h)
│   └── beam_*.h       # Opaque struct declarations & function signatures only. NO struct definitions.
├── src/               # PRIVATE IMPLEMENTATION & INTERNAL HEADERS (.c / _internal.h)
│   ├── core/          # Main entrypoint, initialization, vtable wiring
│   └── <module>/      # Isolated domain modules (e.g., atom, process, message)
├── tests/             # UNIT TESTS & ISOLATION VERIFICATION
│   ├── mocks/         # Simple vtable/dependency mocks
│   └── test_*.c       # Unit test suites executable independently
├── otp_src/           # BEAM / Erlang OTP legacy source code mirror (reference only)
├── CMakeLists.txt     # Strict C11 build configuration (-Wall -Wextra -Wpedantic)
└── AGENT.md           # Engineering guidelines & refactoring protocol
```

---

## 3. Strict Refactoring Principles

1. **ZERO GLOBAL STATE**: Eliminate global variables shared across modules. Encapsulate all state in context structures passed by pointer.
2. **OPAQUE POINTERS**: Public headers in `include/` must NEVER define internal `struct` layouts. Use `typedef struct beam_<module> beam_<module>_t;`.
3. **RESTRICTIVE ENCAPSULATION**: Direct access to struct fields from outside their owning `.c` file is strictly forbidden. Provide explicit accessor functions (e.g., `beam_scheduler_get_status(proc)`).
4. **DEPENDENCY INJECTION (vtables)**: Direct function calls across module boundaries are prohibited. Modules must receive interface structures (*vtables*) during initialization.
5. **BUILD INTEGRITY**: Never break the build. Update `CMakeLists.txt` immediately whenever files are added or modified.

---

## 4. The Refactoring Loop (Iterative Protocol)

For every legacy module refactoring task:

### Step 1: Think (Analysis & Seams Mapping)
- Locate the legacy C file in `otp_src/erts/emulator/beam/`.
- Identify all external header dependencies and cross-module couplings.
- Map struct fields to be hidden behind opaque pointers.
- Define necessary vtable interfaces for external services (e.g., memory allocators, atom tables, system clocks).

### Step 2: Act - Headers (Extract Public API)
- Create `include/beam_<module>.h`.
- Put opaque `typedef struct beam_<module> beam_<module>_t;` and public API function prototypes.
- Keep struct definition inside `src/<module>/beam_<module>_internal.h` or `beam_<module>.c`.

### Step 3: Act - Code (Refactor Implementation)
- Write modular `src/<module>/beam_<module>.c`.
- Replace direct field access with accessor functions.
- Pass dependencies via configuration/vtable structs upon creation/initialization.

### Step 4: Verify & Fix (Compilation & Lint Loop)
- Execute build commands (`cmake -B build && cmake --build build`).
- Read compiler errors (`incomplete type`, `implicit declaration`) carefully.
- Fix remaining coupling iteratively until module compiles cleanly.

### Step 5: Solidify (Mocking & Unit Tests)
- Create dependency mocks under `tests/mocks/mock_<dependency>.h/.c`.
- Write unit tests in `tests/test_<module>.c` verifying isolated behavior.
- Run tests and confirm clean exit code 0.

---

## 5. Communication Standard
When refactoring a module:
1. Summarize the attack plan (which structs become opaque, vtable design).
2. Execute the Refactoring Loop silently using editing & build tools.
3. Present the final result, resolved compiler errors, and proof of passing unit tests.