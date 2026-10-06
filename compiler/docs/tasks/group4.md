# GROUP4 — Shared infrastructure and integration

Owned implementation: All remaining production source files; see `../ownership.json`.

Required work: Shared utilities first; then LLVM parser, MCS/chordality, advanced allocators, DSATUR/ILP, conservative coalescing, allocation verification, backend services, compiler driver, integration and evaluation.

Read `../interfaces.md` and `../testing.md` first. Shared declarations are
coordinated interfaces. Other components are developed by their owners; they
are not assumed complete. Build and run the independent targets:

```sh
cmake -S . -B build
cmake --build build --target utilities-tests lead-tests integration-tests -j 4
```

Run each named executable from `build/`. TODO failures are expected before
implementation. Group 4's shared-utility milestone enables these isolated tests.

Add coverage for: Utility bounds and widths, parser diagnostics, graph proofs, verifier failures, call argument movement, spill/frame correctness and end-to-end execution across allocator budgets.

Submit implementation, tests, a short algorithm/design note, limitations and
validation commands/results. Do not claim final integration from fixture tests.
