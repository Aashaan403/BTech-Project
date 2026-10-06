# Shared interface contract — version 1

These interfaces describe an LLVM-IR-to-RV64IM backend. Clang supplies
source parsing and SSA construction. No source-language frontend is assigned.
The headers in `include/ssarv/` are the authoritative declarations. Changes
to shared declarations require integration-owner coordination before implementation.

## Shared representation

- `ValueId` indexes `Function::values`; `BlockId` indexes `Function::blocks`.
  IDs are dense within their own tables, not interchangeable.
- `kNoValue` and `kNoBlock` are sentinels. Never index a table with them.
- Inputs are structurally valid: labels resolved, predecessor/successor lists
  reciprocal, operands defined, and terminators present where needed.
  Malformed textual IR validation is Group 4's responsibility.
- Empty functions are valid analysis inputs. Nonempty functions enter at block 0.
- `Instr::uses` contains unique register uses per instruction. Repeated uses
  such as `add %x, %x` appear once. Literal operands do not appear there.
- `Instr::type` is the arithmetic/comparison operand width, phi/copy value width,
  or select value width (not its i1 condition). Casts record their source type.
- `operands` preserves order and literals. Phi operands instead live in
  `phiArgs`, whose `pred` identifies the incoming edge. Switch arms live in
  `cases`; `succs[0]` is the default target. Resolved IDs are authoritative
  after transformations; original instruction text and parser `succLabels`
  are diagnostic bookkeeping and need not be regenerated.
- Synthetic `mov` copies carry a register source in `uses[0]`, or a literal
  in `operands[0]` when `uses` is empty. Consumers must not assume a register
  copy has a populated `operands` vector.
- Parameters and instruction destinations comprise `fn.vregs()`. Unreachable
  destinations remain represented, but do not create reachable interference.
- The graph may point to `fn.values` for name comparison. Do not return a graph
  pointing into a temporary/copy that will be destroyed.
- Analysis and allocation receive const inputs and must not mutate them.
  Only the phi-transformation functions mutate IR.

## Group 1 — analysis

Owned file: `src/analysis.cpp`. Group 4 owns bitsets and graph primitives
(`bitset.cpp`, `graph.cpp`) and MCS/chordality (`chordality.cpp`).
Only the primitives are needed for standalone Group 1 tests.

| Function | Required behaviour |
|---|---|
| `reachableBlocks(fn)` | Every block reachable from entry exactly once; empty for an empty function. Deterministic order, entry first; exact DFS tie choices are not graded. |
| `reversePostorder(fn)` | Reverse of a deterministic DFS postorder over reachable blocks, entry first. Loop back edges need not respect forward order. |
| `dominators(fn)` | Vector of `blocks.size()` immediate dominators; entry maps to itself; unreachable blocks map to `kNoBlock`. |
| `dominates(idom,a,b)` | Whether `a` occurs on `b`'s immediate-dominator chain; reflexive for reachable blocks, false for out-of-range/unreachable IDs. |
| `loopDepths(fn)` | Count distinct natural-loop headers containing each block. A back edge targets a dominator of its source. Union loop bodies sharing one header before counting; two latches are one loop. Unreachable/outside blocks have depth zero. Irreducible-cycle nesting is outside version 1. |
| `computeLiveness(fn)` | Vectors indexed by block ID, each bitset sized to `numValues()`. Unreachable sets are empty. Incoming phi operands are live on their own predecessor edges, never all arms together. |
| `interferenceGraph(fn)` | Undirected, symmetric graph of `numValues()` capacity, no self edges; nodes include `fn.vregs()`. Definitions conflict with distinct values live after them; phi results conflict at the phi boundary if live. Parameters conservatively conflict with each other and with values live at entry. Return maximum live-set size under this convention. Empty-function pressure is zero. |

**Liveness boundary convention:** for a consistent boundary across analysis
consumers, `liveIn[b]` describes the entry boundary with phi destinations present.
It includes all phi destinations. For an edge `p -> b`, subtract the destinations
of `b`'s phis from `liveIn[b]`, then add only phi operands arriving from `p`.
Inside a block, process ordinary instructions backward: kill destination, add
uses. Phi operands are not ordinary uses during this walk. This boundary convention
does not mean a phi destination is used in its predecessor.

Conservative parameter handling and dead definitions can differ from a pruned
SSA graph. Chordality and `omega == MAXLIVE` are benchmark checks, not a substitute
for manually checked live sets and edges on arbitrary input.

## Group 2 — classical allocation

Owned file: `src/allocators.cpp`. Group 4 owns advanced allocators and
registry (`advanced_allocators.cpp`), result helpers (`alloc_result.cpp`)
and ILP (`ilp.cpp`). Standalone allocator tests substitute explicit
analysis fixtures; production uses Group 1 analysis.

| Function | Required behaviour |
|---|---|
| `spillCosts(fn)` | One nonnegative cost per value. Every reachable instruction definition and unique register use contributes `10^loopDepth`; each parameter contributes one for its entry definition. Phi uses follow `Instr::uses` for this static proxy; it is not dynamic reload frequency. |
| `allocNaive(fn,g,k)` | Spill every graph node; allocate no registers. |
| `allocLinearScan(fn,g,k)` | Flatten reachable CFG into reverse-postorder intervals, account for liveness across blocks, allocate available registers, and use interval end points to choose whole-value spills. |
| `allocChaitinBriggs(fn,g,k)` | Simplify low-degree nodes, optimistically select spill candidates using cost/degree, then select colours backward; spill when no colour is available. |

- Accepted register budgets are **1..25**; reject others with
  `std::invalid_argument`. An empty graph still validates the budget.
- `assignment.size() == fn.numValues()`; every graph node gets a decision.
- A colour is in `[0,k)`. `kSpill == -1` means memory. `kUnassigned == -2`
  is permitted only for values absent from the graph.
- `spilled` contains each spilled graph value exactly once and agrees both
  ways with `assignment`. Nonspilled adjacent nodes have different colours.
- `name` is `naive`, `linear_scan`, or `chaitin_briggs`; `k` matches input.
- `exact` is false for these heuristics. `ops` records consistently documented
  algorithm work; it is not timing and is not compared for exact equality.
- No live-range splitting, assembly emission, or spill insertion is assigned.
- Different valid colour assignments/spill choices are permitted. Algorithm
  fidelity, validity, reproducibility, and measured cost matter, not matching
  particular prescribed register names.

## Group 3 — phi transformation and lowering

Owned files: `src/phielim.cpp`, `src/lowering.cpp`. Memory operations, calls,
spill slots, stack frames, globals, register mapping, and support validation
are implemented by Group 4 in `backend.cpp` and `driver.cpp`.
Standalone phi tests supply declared reachability; lowering tests supply
a recording context. Production phi handling can use Group 1 traversal.

| Function | Required behaviour |
|---|---|
| `sequentialise(pairs,makeTemp)` | Return sequential copies with simultaneous-assignment semantics: every source means its old value. Remove self copies; break cycles using fresh IDs from the callback. Destination IDs in the input are unique; sources may repeat. The callback always returns unused IDs. |
| `splitCriticalEdges(fn)` | Split edges whose source has multiple successors and target multiple predecessors. Keep block IDs dense, label map, reciprocal CFG links, branch targets, switch cases, and phi predecessor IDs/labels consistent. Return number split. |
| `eliminatePhis(fn)` | Split critical edges, insert edge copies before predecessor terminators, handle literals and cycles, and remove reachable phis. Preserve the phi type on generated copies (including cycle temporaries). Generated names must not collide with existing values. Report actual split/phi/copy/cycle counts. Applying it again inserts no further phi copies. |
| `lowerBinary(in,ctx)` | Integer add/sub/mul, signed/unsigned div/remainder, bit operations, and shifts. Respect source width and wraparound where defined. |
| `lowerCompare(in,ctx)` | All ten integer predicates; distinguish signed/unsigned comparison; result is 0 or 1. |
| `lowerCast(in,ctx)` | `trunc`, `zext`, `sext`; `in.type` is the source width, target follows ` to ` in `in.text`. Truncation to i1 keeps the low bit, not whether the value is nonzero. |
| `lowerSelect(in,ctx)` | Select exactly the chosen value, including destination/source aliasing and spilled operands. |
| `lowerMove(in,ctx)` | Register or literal copies generated by phi elimination; commit results through context. |
| `lowerTerminator(in,ctx)` | Conditional/unconditional branches, switch, ret, unreachable. Returns jump to the context epilogue; no stack-frame handling. |

Basic integer lowering supports widths **1..64**, including i33 intermediates.
Use only `t5`/`t6` as scratch. Values use their low width bits; i1 results are
0/1, and ordinary narrower integer results are sign-extended in their physical
register representation. Casts must preserve the requested bit semantics.
Poison/undefined-operation cases need not produce a specified result. Floating
point, vectors, and wider-than-64 integer operations are outside version 1.

`LoweringContext` service guarantees:

- `operand(op,scratch)` materialises an operand, returning its physical register.
  It may overwrite the requested scratch, but no other allocated register.
  Literals are materialised without implicit instruction-width conversion.
- `destination(value,scratch)` returns the allocated register, or requested
  scratch for a spilled result. It does not emit code or overwrite a source.
- `storeResult(value,reg)` commits a spilled result; allocated results need no
  store. Call it once for each value-producing lowering operation.
- `blockLabel`, `epilogueLabel`, `freshLabel` provide labels; local labels are
  unique for the function. No static/global counters are needed in student code.
- `emit(text)` appends assembly. Lowering may not change `sp` or implement a
  function prologue/epilogue.
- A destination can alias a source. Read the needed operands before overwriting
  an allocated register. Reading a second operand may overwrite its scratch.

The planned executable pipeline eliminates phis **before** allocation.
The SSA study will run allocation on the original graph separately.
Coalescing is a separate analysis experiment in interface version 1.

## Shared utility contract — Group 4, milestone 2

- Value interning gives stable dense IDs and does not insert on lookup.
- IR helpers classify terminators, enumerate phis and count instructions.
  `vregs()` returns sorted unique parameters and instruction destinations.
- Bitsets have fixed width. Set/reset/test reject out-of-range indices;
  binary operations require equal widths. Iteration is ascending and never
  includes padding bits. Equality includes the width.
- Graph capacity is fixed. Edges are undirected, idempotent and activate
  endpoints. Self edges are ignored. Explicit isolated nodes are supported.
  Invalid IDs throw `std::out_of_range`, including sentinels.
- Graph name tie-breaking borrows the supplied pool: its lifetime must exceed
  graph use. Graphs without a pool compare IDs.
- `AllocResult::coloursUsed()` counts distinct nonnegative colours.

## Group 4 — remaining interfaces

`parser.cpp`: parse the agreed LLVM subset into resolved, internally consistent
IR and return diagnostic errors using `std::expected`. Add supported syntax
incrementally; reject unsupported syntax instead of silently approximating it.
`typeSizeBytes` computes storage for supported scalar/aggregate LLVM types.

`chordality.cpp`: MCS, perfect elimination verification, chordality and clique
size (the latter exact only for chordal graphs). Advanced allocation and ILP
use the same allocation validity contract as Group 2. Set `exact` only when a
result is proven; report fallback/time-limit behaviour truthfully.

`coalesce.cpp`: conservative merging with a representative map and statistics.
`verify.cpp`: independently validate allocation sizes, colours, graph conflicts
and spill-list consistency. `backend.cpp`: implement `LoweringContext`, register
mapping, memory operations, calls, stack frames, globals and spill management.
The ABI colour order is fixed in the header/backend skeleton. Reserve t5/t6.
Initially support at most eight direct-call arguments/parameters, integer
operations of width 1..64 and representable stack frames; explicitly reject
unsupported operations. Expand coverage through a shared interface proposal.

`driver.cpp`: implement command-line handling and orchestrate parsing, phi
elimination, analysis, allocation, verification, emission and evaluation.
The compiler must expose clear errors rather than emit partial output after
failure. Add file/output interfaces and execution tooling during integration.

## Acceptance and integration

Run each component's tests independently. Add tests and a short design note.
Fixture tests check examples and invariants; they do not prove full correctness.
Unfinished acceptance targets are explicitly disabled in default CTest runs.
A compiler is ready only when production analysis replaces fixture providers
and enabled integration/execution checks pass. See `testing.md` for commands.
