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

## Algorithm and Design Note

Group 1 owns SSA CFG traversal, dominance, natural-loop depth analysis, phi-aware liveness analysis, and interference graph construction in `src/analysis.cpp`.

1. **CFG Traversal and Reverse Postorder (`reachableBlocks`, `reversePostorder`)**:
   - Traversal uses an explicit heap-allocated DFS stack (`std::vector<Frame>` storing `BlockId` and `nextSucc` index) rather than recursion. This eliminates call stack overflow risk on large or deep CFGs (tested on deep chains of 50,000+ basic blocks).
   - `reachableBlocks` records reachable blocks in deterministic DFS preorder starting from `fn.entry()`. Unreachable blocks and disconnected cycles are omitted.
   - `reversePostorder` performs postorder numbering upon exploring all successors of each node and reverses the result, guaranteeing that `fn.entry()` appears first.

2. **Immediate Dominators and Dominance Queries (`dominators`, `dominates`)**:
   - Computes immediate dominators via the Cooper-Harvey-Kennedy iterative dataflow algorithm using reverse postorder numbering.
   - The entry block maps to itself (`idom[entry] == entry`). Unreachable blocks map to `kNoBlock`.
   - Intersect operations advance two fingers upward along the immediate dominator tree based on reverse postorder indices until convergence.
   - `dominates(idom, a, b)` walks the `idom` chain from `b` upwards to determine whether `a` dominates `b`; it is reflexive for reachable blocks and returns `false` for invalid block IDs or unreachable blocks.

3. **Natural Loop Depths (`loopDepths`)**:
   - Detects loop back-edges `u -> h` where header `h` dominates latch `u` (`dominates(idom, h, u)`).
   - Multi-latch loops: Multiple latches branching to the same header `h` are grouped into a single natural loop.
   - Computes loop bodies iteratively by propagating a predecessor worklist starting from all latches backwards to header `h`.
   - Each block's nesting depth is the count of distinct natural loops that contain it (0 for non-loop blocks).

4. **Phi-Aware Liveness Analysis (`computeLiveness`)**:
   - Computes dense `liveIn` and `liveOut` bitsets per block through backward dataflow iteration over reverse postorder until a fixpoint is reached.
   - Phi edge transfer semantics: For each successor edge `b -> s`, phi destination values defined in `s` are cleared from the incoming live set along that edge, and incoming phi operand values associated with predecessor `b` are inserted.
   - Intra-block transfer: Walks instructions backward from terminator to entry, removing instruction definitions from the live set and adding their operand uses. LiveIn is updated to incorporate all instruction uses and block phi definitions.

5. **Interference Graph and Live Pressure (`interferenceGraph`)**:
   - Constructs an undirected graph with capacity `fn.numValues()`, attaches the function's `ValuePool` for deterministic tie-breaking, and adds all `fn.vregs()` as nodes.
   - Empty functions (`fn.blocks.empty()`): Preserves `fn.numValues()` capacity and registers all `fn.vregs()` nodes (including function parameters) with pressure 0 and no interference edges.
   - Formal parameters conservatively conflict with each other and with all values live at function entry.
   - Instruction definitions conflict with all distinct values live immediately after the defining instruction.
   - Active phi results conflict with distinct values live at the phi boundary.
   - Tracks maximum register pressure (`maxlive`) across entry, instruction definitions, and phi boundaries.

## Limitations

- **Irreducible Control Flow**: Back-edge identification relies on dominator trees (`dominates(idom, h, u)`), which identifies natural loops in reducible CFGs. Irreducible multi-entry loops require loop restructuring or node-splitting before natural loop nesting can be computed.
- **Dead Instructions**: Dead instructions within reachable blocks are not eliminated prior to liveness analysis; their operands are treated as live up to their definition point unless an optimization pass eliminates them.
- **Matrix Memory Scalability**: The interference graph utilizes dense `BitSet` adjacency rows. This provides fast bitwise queries ($O(1)$) and cache-friendly operations, scaling well for functions up to thousands of values, but uses $O(V \cdot N)$ bitset memory.

## Validation Commands and Results

Build configuration with analysis checks enabled:

```sh
cmake -S compiler -B compiler/build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug -DSSARV_TEST_ANALYSIS=ON
cmake --build compiler/build --target analysis-tests -j 4
ctest --test-dir compiler/build -L group1 --output-on-failure
```

CTest execution summary:

```
Test project D:/BTech-Project/compiler/build
    Start 3: contract.analysis
1/1 Test #3: contract.analysis ................   Passed    0.08 sec

100% tests passed out of 1

Label Time Summary:
group1    =   0.08 sec*proc (1 test)

Total Test time (real) =   0.10 sec
```

Direct unit test binary results (`./compiler/build/analysis-tests`):

- `Analysis.EmptyFunction`: Passes (0 blocks, empty reachability/dominators/liveness, 0 maxlive, 0 nodes).
- `Analysis.EmptyFunctionWithParametersPreservesCapacityAndVregs`: Passes (preserves `fn.numValues()` capacity and `fn.vregs()` nodes with pressure 0 and 0 edges).
- `Analysis.DeepCFGLargeGraphNoStackOverflow`: Passes (traverses 50,000 basic blocks iteratively without stack overflow).
- `Analysis.DiamondDominatorsTraversalAndPhiEdges`: Passes (validates diamond CFG reachability, RPO, immediate dominators, liveness, and interference).
- `Analysis.UnreachableBlocksAndInvalidDominanceQueries`: Passes (ensures unreachable blocks are omitted from traversal and map to `kNoBlock`).
- `Analysis.TwoLatchesCountAsOneNaturalLoop`: Passes (verifies multi-latch loops sharing one header are counted as one loop).
- `Analysis.NestedLoopsDepthCalculation`: Passes (calculates nesting depths across outer and inner loops correctly).
- `Analysis.MultiplePhisInJoinBlockLiveInterferenceAndDeadPhi`: Passes (verifies phi boundary interference and dead phi exclusion).
- `Analysis.UnreachableCycleRemainsIsolated`: Passes (ensures disconnected cycles have 0 loop depth, 0 liveness, and 0 degree).
- `Analysis.ManuallyVerifiedInterferenceEdgesAndMaxLive`: Passes (validates precise instruction-level conflict edges and maximum live register pressure).

