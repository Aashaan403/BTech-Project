# GROUP2 — Classical register allocation

Owned implementation: `src/allocators.cpp`.

Required work: Loop-weighted spill costs, naive allocation, whole-interval linear scan and optimistic Chaitin–Briggs allocation.

Read `../interfaces.md` and `../testing.md` first. Shared declarations are
coordinated interfaces. Other components are developed by their owners; they
are not assumed complete. Build and run the independent targets:

```sh
cmake -S . -B build
cmake --build build --target allocators-tests -j 4
```

Run each named executable from `build/`. TODO failures are expected before
implementation. Group 4's shared-utility milestone enables these isolated tests.

Add coverage for: Budgets 1..25, invalid budgets, empty graphs, noncliques, tight pressure, ample budgets, loop weights and determinism. Check algorithm fidelity as well as validity.

Submit implementation, tests, a short algorithm/design note, limitations and
validation commands/results. Do not claim final integration from fixture tests.
