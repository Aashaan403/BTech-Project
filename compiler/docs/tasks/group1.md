# GROUP1 — Program analysis

Owned implementation: `src/analysis.cpp`.

Required work: Traversal, reverse postorder, immediate dominators, dominance queries, natural-loop depths, phi-aware liveness, interference and maximum live pressure.

Read `../interfaces.md` and `../testing.md` first. Shared declarations are
coordinated interfaces. Other components are developed by their owners; they
are not assumed complete. Build and run the independent targets:

```sh
cmake -S . -B build
cmake --build build --target analysis-tests -j 4
```

Run each named executable from `build/`. TODO failures are expected before
implementation. Group 4's shared-utility milestone enables these isolated tests.

Add coverage for: Empty/unreachable blocks, diamonds, phi edges, loops with multiple latches, nested loops and manually checked live sets/edges.

Submit implementation, tests, a short algorithm/design note, limitations and
validation commands/results. Do not claim final integration from fixture tests.
