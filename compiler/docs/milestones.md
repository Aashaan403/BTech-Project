# Development milestones

## 1. Interfaces and starter

Common headers, all four groups' implementation stubs, component libraries,
manual IR/graph fixtures, recording lowering context and acceptance targets.
The entire tree builds. Only the scaffold check is enabled by default.
No compiler execution is claimed at this milestone.

## 2. Shared utilities

Group 4 implements `ir_support.cpp`, `bitset.cpp`, `graph.cpp` and
`alloc_result.cpp`, and BitSet iteration. Enable utility acceptance checks.
Status: shared utilities complete; scaffold and utility checks enabled.
Groups 1–3 can now run their isolated tests without a parser, advanced
allocator or real backend. Their TODO functions still fail acceptance.

## 3. Parallel component development

Group 1 implements analysis. Group 2 implements classical allocation using
fixture analysis during local development. Group 3 implements phi handling
and basic lowering using a recording context. Group 4 develops parser,
chordality, advanced algorithms, verification and the remaining backend.
Submit actual implementation and tests in feature branches.

## 4. Integration

Link Group 2 against production Group 1 analysis. Connect the parser,
phi transformation, allocation, verification and emitter through the driver.
Keep original-SSA experiments separate from the post-phi executable pipeline.
Enable all component and integration checks. Add host-versus-RISC-V execution
comparisons with Clang, a RISC-V cross compiler and QEMU. Missing tools must
be reported explicitly rather than counted as passing execution checks.

## 5. Evaluation and review

Measure allocation validity, spill costs, chordality, operation counts,
code size and execution outcomes across budgets and benchmarks. Document
unsupported inputs and proof/fallback flags. Produce reproducible commands
and review material from the integrated compiler's measured results.
