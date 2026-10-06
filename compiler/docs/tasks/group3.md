# GROUP3 — Phi elimination and basic lowering

Owned implementation: `src/phielim.cpp`, `src/lowering.cpp`.

Required work: Parallel-copy scheduling, critical-edge splitting, phi elimination and integer instruction/terminator lowering through LoweringContext.

Read `../interfaces.md` and `../testing.md` first. Shared declarations are
coordinated interfaces. Other components are developed by their owners; they
are not assumed complete. Build and run the independent targets:

```sh
cmake -S . -B build
cmake --build build --target phi-tests lowering-tests -j 4
```

Run each named executable from `build/`. TODO failures are expected before
implementation. Group 4's shared-utility milestone enables these isolated tests.

Add coverage for: Self copies, chains, cycles, literals, critical switch edges, idempotence and type preservation. Lowering needs widths 1..64, unsigned operations, casts, comparisons and destination/source aliasing.

Submit implementation, tests, a short algorithm/design note, limitations and
validation commands/results. Do not claim final integration from fixture tests.
