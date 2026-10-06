# SSA register allocation and RV64IM compiler

Team development repository for an LLVM IR subset parser, SSA analysis,
register allocation experiments, phi elimination and RISC-V emission.
Clang supplies source parsing and SSA construction.

Read [interfaces](docs/interfaces.md), [milestones](docs/milestones.md),
[testing](docs/testing.md), and the task descriptions in `docs/tasks/`.
All four groups begin with implementation stubs. The executable builds but
reports a TODO until orchestration and compiler components are implemented.

Current milestone: shared utilities are implemented and their checks are enabled.
Analysis, allocators, phi elimination, lowering and the remaining lead components
are still TODOs.

## Build

Requires CMake >= 3.24, a C++23 compiler/library with `<expected>`, and
GoogleTest when testing is enabled. No dependencies are downloaded by CMake.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j 4
ctest --test-dir build --output-on-failure
```

Acceptance checks for unfinished components are listed as disabled; this
means pending implementation, not a passing compiler. Run the component
executables directly while developing, or enable their CMake switches.

## Components

- Group 1: program analysis.
- Group 2: classical register allocation.
- Group 3: phi elimination and basic lowering.
- Group 4: shared utilities, parser, advanced algorithms,
  remaining backend, orchestration, integration and evaluation.

Components use common headers. Allocator unit tests use explicit analysis
fixtures instead of the production analysis library. Phi and lowering tests
construct IR directly, without requiring a parser or backend.
