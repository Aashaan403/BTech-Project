# BTP Phase 1 — Review 1: Slide Content

**Group:** 4 members · **Guide:** Dr. Nandini J Warrier · **Date:** 9 September 2026, early slot
**Format:** 15 min presentation + 5 min Q&A · **Template:** institute PPT (8 slides), extended to 15

**Evaluation criteria this deck must hit:**
(a) problem definition and scope of work · (b) literature survey · (c) proposed architecture/methodology · (d) applications of proposed work and presentation skills

---

## Section assignments

Four sections, roughly equal speaking time.

| Section | Slides | Time | Presenter | Character of the material |
|---|---|---|---|---|
| **A** | 1, 2, 4, 5 | ~3:45 | | Framing and literature. Opens the review and sets its tone; the most scriptable section in the deck. |
| **B** | 3, 6, 7 | ~4:00 | **Team lead** | The conceptual core — SSA, chordality, the research gap. Draws the most follow-up questions, so it sits with the lead, who also anchors Q&A. |
| **C** | 8, 9, 10 | ~4:00 | | Scope and system design. Concrete and diagram-driven; the architecture slide carries a full rubric criterion. |
| **D** | 11, 12, 13, 14, 15 | ~3:15 | | Evaluation, applications, plan. Five slides, but each is short and list-shaped. |

Fill in the Presenter column tonight and circulate it, so nobody arrives unsure which slides are theirs. The team lead's separate brief is in `lead-brief.md`.

**Handover rule:** each presenter ends with one sentence naming what comes next and who is presenting it. "That's the problem we're solving — [name] will now take you through the scope and the system architecture." Silent swaps read as disorganised.

---

# Slide 1 — Title

**On the slide**

> ### Graph-Theoretic Register Allocation in an SSA-Based Compiler Backend
> #### Design, Implementation, and the Empirical Limits of Chordal Colouring
>
> **Presented by:** [Member 1] · [Member 2] · [Member 3] · [Member 4]
> **Guided by:** Dr. Nandini J Warrier
> Department of Computer Science and Engineering
> Indian Institute of Information Technology, Kottayam

**Speaker notes (Section A) — 20 s**

Good evening. We are presenting our B.Tech project, *Graph-Theoretic Register Allocation in an SSA-Based Compiler Backend*, guided by Dr. Nandini Warrier. Over the next fifteen minutes the four of us will cover the problem, the existing literature, the gap we identified, our proposed architecture, and our evaluation plan.

> **Note:** Confirm the exact title with the guide before printing. `scope.md` §1 lists two shorter alternatives if she prefers a tighter title.

---

# Slide 2 — Introduction

**On the slide**

**Register allocation: the last decision every compiler makes**

- A program manipulates unboundedly many named values; a CPU has a fixed, small set of registers — 32 on RV64I, 16 on x86-64
- The compiler backend must map the first onto the second. Values that do not fit are **spilled** to memory
- A spill is a load and a store on every access — inside a loop, that cost is paid on every iteration
- Register allocation is consistently among the highest-impact backend passes: the difference between a good and a poor allocator is routinely tens of percent of execution time
- Classical formulation (Chaitin, 1981): build an **interference graph**, colour it with *k* colours, where *k* = number of registers

**Speaker notes (Section A) — 55 s**

Every program a compiler sees uses far more values than the machine has registers. RISC-V gives us thirty-two; a realistic function may have hundreds of simultaneously meaningful values. Deciding which live in registers and which get pushed out to memory is register allocation, and it is the last significant decision the backend makes before emitting code.

It matters because the cost is not paid once. A value spilled to memory costs a load and a store *every time it is touched* — and if that access sits inside a loop, the compiler pays that on every iteration. Register allocation is reliably one of the two or three highest-impact optimisations in a backend.

Chaitin, in 1981, gave the formulation everybody still teaches: build a graph where each value is a node and an edge means two values are live at the same time, then colour that graph with as many colours as you have registers. That reduction is the foundation of the whole area — and it is also the source of the claim we are going to examine.

---

# Slide 3 — Background: SSA and Chordality

**On the slide**

**Static Single Assignment (SSA) form**
- Every value is assigned exactly once; φ-functions merge values at control-flow join points
- *Strict* SSA: every use of a value is dominated by its definition
- Universal in modern compilers — LLVM, GCC, HotSpot, V8

**The structural consequence**
- Under strict SSA, each value's live range is a connected subtree of the dominator tree
- The interference graph is therefore an **intersection graph of subtrees of a tree** — which is exactly the class of **chordal graphs**

**Why that matters**
- Chordal graphs admit a **perfect elimination ordering (PEO)**, found in linear time by Maximum Cardinality Search (Tarjan & Yannakakis, 1984)
- Greedy colouring along the reverse PEO uses exactly **ω** colours — provably optimal
- And **ω = MAXLIVE**: the minimum registers needed, absent spilling, is computable in linear time

> *Suggested figure:* a small CFG with φ-nodes → its dominator tree → the resulting interference graph, annotated as chordal. One diagram, three panels.

**Speaker notes (Section B) — 80 s**

To explain our contribution I need thirty seconds of background on SSA form.

In SSA, every value is assigned exactly once, and where control flow merges we insert a φ-function to pick the right incoming value. Strict SSA adds one condition: every use of a value is dominated by its definition — you can never reach a use without passing through the definition first. This is not an exotic form; it is what LLVM, GCC, HotSpot and V8 all use internally.

The structural consequence is the important part. Under strict SSA, the region of the program where a value is live forms a connected subtree of the dominator tree. So the interference graph is an intersection graph of subtrees of a tree — and that is precisely the definition of a chordal graph.

Chordal graphs are extremely well-behaved. They have a perfect elimination ordering, which Tarjan and Yannakakis showed in 1984 can be found in linear time by maximum cardinality search. Colour greedily along the reverse of that ordering and you use exactly ω colours — the clique number — which is provably the minimum. Not a heuristic. The optimum, in linear time.

And there is a clean corollary: for strict SSA, ω equals MAXLIVE, the maximum number of values live at any point. So the minimum register count you need, if you never spill, is a linear-time computation.

> **Anticipate here:** if the panel is unfamiliar with chordality, this is where you lose them. Define it in one line on the way past — *"a graph where every cycle of length four or more has a chord"* — and move on. Do not stop to prove anything.

---

# Slide 4 — Literature Review (1/2): The Classical Line

**On the slide**

| Work                                                                            | Year | Contribution                                                                                                                               | Limitation we build on                                                 |
| ------------------------------------------------------------------------------- | ---- | ------------------------------------------------------------------------------------------------------------------------------------------ | ---------------------------------------------------------------------- |
| Chaitin et al., *Register allocation via colouring*                             | 1981 | Interference-graph formulation; simplify rule for nodes of degree < *k*; showed **any** graph can arise as an interference graph ⇒ NP-hard | Hardness argued for arbitrary programs, not SSA programs               |
| Briggs, Cooper & Torczon, *Improvements to graph colouring register allocation* | 1994 | **Optimistic colouring** — defer the spill decision to the select phase; conservative coalescing; rematerialisation                        | Still a heuristic; no optimality guarantee even where one is available |
| George & Appel, *Iterated register coalescing*                                  | 1996 | Interleave coalescing with simplify under a conservative test; removes copies without inducing spills                                      | Coalescing remains NP-hard; quality is not characterised               |
| Poletto & Sarkar, *Linear scan register allocation*                             | 1999 | Live intervals over a linear ordering; O(*n* log *k*); the standard fast allocator for JITs                                                | Trades allocation quality for compile speed                            |

**Speaker notes (Section A) — 65 s**

The literature divides cleanly into two lines. This is the classical one.

Chaitin's 1981 paper is the origin. It gives the interference-graph reduction, the simplify rule — remove any node with fewer than *k* neighbours, because it can always be coloured later — and, critically, the hardness argument: Chaitin showed that *any* graph can be realised as the interference graph of some program, so colouring interference graphs inherits the NP-hardness of graph colouring. That single result is why every textbook presents register allocation as intractable.

Briggs and colleagues improved it in 1994 with optimistic colouring — rather than spilling a high-degree node immediately, push it onto the stack anyway, because a colour may still be free when you pop it. That is the allocator most production compilers descended from, and it is the one we implement as our classical comparison point.

George and Appel added iterated coalescing in 1996, which removes register-to-register copies safely. Poletto and Sarkar's linear scan, in 1999, gave up graph colouring entirely for speed — it is what just-in-time compilers use when compile time is on the critical path.

What all four share is that they are heuristics, and none of them ask whether the hardness result they rest on actually applies to the programs they are given.

---

# Slide 5 — Literature Review (2/2): The SSA Line, and the Gap

**On the slide**

| Work | Year | Contribution |
|---|---|---|
| Tarjan & Yannakakis, *Simple linear-time algorithms to test chordality* | 1984 | Maximum Cardinality Search — linear-time chordality test and PEO construction. Our principal tool |
| Hack & Goos, *Register allocation for programs in SSA form* | 2005–06 | **Interference graphs of strict SSA programs are chordal** ⇒ optimal colouring in linear time; decouples spilling from assignment |
| Pereira & Palsberg, *Register allocation via coloring of chordal graphs* | 2005 | Reports ~95% of methods in the Java standard library have chordal interference graphs; a chordal allocator is competitive with iterated coalescing |
| Bouchez, Darte & Rastello, *What does the NP-completeness proof of Chaitin et al. really prove?* | 2006 | The hardness lies in **spilling under a fixed register budget**, not in colouring. Chaitin's construction requires leaving SSA |

**The gap**

- Theory (Hack & Goos; Bouchez et al.) is settled; measurement inside a *working backend* is not
- Pereira & Palsberg measure chordality *frequency*; nobody quantifies **how much colouring quality is actually lost** by the classical heuristic
- Real backends **break the SSA precondition routinely** — φ-elimination, ABI pre-colouring, register aliasing — and the resulting degradation is undocumented
- No published study sweeps the **register budget** to test whether the structural advantage survives real pressure

**Speaker notes (Section A) — 70 s**

The second line of work overturns the first.

Tarjan and Yannakakis, in 1984, gave the algorithm we depend on: maximum cardinality search tests chordality and produces a perfect elimination ordering, both in linear time.

Hack and Goos, in 2005, proved the result at the centre of our project — for programs in strict SSA form, the interference graph is chordal. Colouring is therefore in P. Pereira and Palsberg independently reported that around ninety-five percent of methods in the Java standard library have chordal interference graphs, and that a chordal allocator is competitive with iterated register coalescing.

Bouchez, Darte and Rastello then pinned down exactly what Chaitin's proof establishes. Their answer: the hardness is in *spilling* — deciding what to evict when the register budget is fixed — and in coalescing. Not in colouring. Chaitin's construction implicitly requires programs that are not in SSA form.

So the theory is settled. What is not settled is the measurement. Pereira and Palsberg tell us how *often* graphs are chordal, but not how much quality the classical heuristic actually gives up when they are. Nobody has documented how badly real backend transformations — φ-elimination, ABI-mandated pre-coloured registers, register aliasing — destroy the chordality that SSA guarantees. And no study sweeps the register budget to see whether the advantage survives once spilling dominates.

That is the space our project occupies.

---

# Slide 6 — Motivation

**On the slide**

**The textbook account is wrong in a specific, measurable way**

> *"Register allocation is NP-hard because it reduces to graph colouring."*
> — the standard justification for using heuristics

- True for arbitrary programs. **False for programs in strict SSA form** — which is the form every modern compiler already uses
- Yet production allocators (LLVM `greedy`, GCC IRA) remain heuristic graph-colouring descendants of Chaitin–Briggs
- **So: is the industry leaving something on the table, or is the theoretical advantage worth nothing in practice?**

**Nobody has answered that empirically inside a real backend.** That is the gap.

**Our position:** we do not assume the answer is "yes". A negative result — *the heuristic is already near-optimal, and the theory buys nothing* — is a legitimate and publishable-quality finding, and our evaluation is designed to establish it either way.

**Speaker notes (Section B) — 65 s**

Here is why this is worth doing.

Every compiler textbook justifies heuristic register allocation by pointing at NP-hardness. That justification is correct for arbitrary programs and incorrect for programs in strict SSA form — which is the form essentially every modern compiler has already put its code into by the time it reaches the allocator.

And yet production allocators have not moved. LLVM's `greedy` allocator and GCC's IRA are both heuristic descendants of Chaitin and Briggs. There are good engineering reasons they might be — but the question of what is actually being given up has not been measured.

So the motivating question is simple to state: is the compiler industry leaving quality on the table by ignoring chordality, or is the theoretical advantage worth essentially nothing once you meet a real machine?

I want to be explicit with the panel about one thing. We are not assuming the answer is favourable. Our honest expectation is that the classical heuristic is close to optimal most of the time. If that is what we find, it is a real result — it tells you the theory does not translate — and our second and third research questions are designed to be independently informative regardless. The project does not depend on any particular outcome.

> **This candour is deliberate.** A panel that spots an unfalsifiable project will attack it. Stating the null result as an acceptable outcome removes that line of attack entirely and reads as methodological maturity.

---

# Slide 7 — Problem Statement

**On the slide**

**Design and implement an SSA-based compiler backend targeting RISC-V, containing four register allocation strategies, and use it to answer three measurable questions.**

**RQ1 — How often, and by how much, is the classical heuristic suboptimal at colouring?**
Chaitin–Briggs is a heuristic; on a chordal graph the optimum is available in linear time. Measure: colours used by Chaitin–Briggs vs. ω from the PEO, across every benchmark function.

**RQ2 — Where does chordality break down in a real backend?**
Strict SSA guarantees chordality; a working backend violates the preconditions routinely — φ-elimination inserts copies and destroys SSA, ABI pre-colouring adds edges, register aliasing makes interference non-unit. Measure: chordality via MCS at each pipeline stage; characterise which transformation destroys it and how severely.

**RQ3 — Does chordality confer any advantage once registers are scarce?**
Colouring is easy only while *k* ≥ ω. Below that, spilling is required — and spilling is NP-hard even on chordal graphs. Measure: sweep the register budget *k* ∈ {8, 12, 16, 24, 32} across all four allocators.

**Speaker notes (Section B) — 75 s**

That motivation becomes three concrete research questions, and every one of them is answered by a counted quantity rather than an opinion.

The first: how often is the classical heuristic actually suboptimal? Chaitin–Briggs makes a greedy choice with no guarantee. On a chordal graph we can compute the true optimum cheaply. So we run both on the same graph and compare colour counts. The gap is directly measurable across every function in our benchmark set.

The second is the one we think is most likely to produce something new. SSA guarantees chordality — but a backend that actually emits machine code has to leave SSA. φ-elimination inserts copy instructions and destroys the single-assignment property. The calling convention forces specific values into specific physical registers, which adds interference edges that were not there. Register aliasing means interference is not always a simple edge. Every one of these is a place where the guarantee fails, and to our knowledge nobody has quantified which one does the most damage. We test chordality with maximum cardinality search at each stage of the pipeline and attribute the breakage.

The third: the chordality advantage exists only while you have at least ω registers. Below that you must spill, and spilling is NP-hard even on chordal graphs. Because our allocator takes the register count as a parameter, we can artificially cap it and sweep — eight, twelve, sixteen, twenty-four, thirty-two — and watch whether the structural advantage survives contact with real pressure. That sweep costs us nothing, because it is a parameter we already have.

---

# Slide 8 — Scope of Work

**On the slide**

**In scope**

| Layer | Components |
|---|---|
| Input | Textual LLVM IR (`.ll`) parser for a defined subset |
| Analysis | CFG construction · dominator tree · natural loop detection with nesting depth · liveness analysis · interference graph construction · chordality testing (MCS) |
| Allocation | Four allocators: naive · linear scan · Chaitin–Briggs · SSA/chordal · plus iterated register coalescing |
| Backend | φ-elimination with correct parallel-copy semantics · spill code insertion · RISC-V calling convention · textual assembly emission |
| Validation | Static allocation verifier · differential testing against reference outputs · Csmith random-program testing |
| Evaluation | Metric collection · `llc` baselines · register-budget sweep · plots · interference-graph visualiser |

**Explicitly out of scope** — *stated to prevent scope drift, not to reduce ambition*

- Any optimisation beyond register allocation — no GVN, no LICM, no vectorisation, no instruction scheduling (`clang -O1` performs these before we receive the IR)
- Instruction selection beyond direct one-to-one lowering
- Floating point · structs passed by value · varargs · C++ features · exceptions
- Binary encoding and linking — we emit assembly text; `riscv64-linux-gnu-gcc` assembles and links
- **Outperforming LLVM's `greedy` allocator is not a goal.** Reaching within a constant factor and *accounting for the difference* is the goal

**Speaker notes (Section C) — 70 s**

This slide is our defence against scope creep, and I want to be equally clear about both halves of it.

In scope: an LLVM IR parser, a full analysis layer — control flow graph, dominator tree, loop detection, liveness, interference graph construction, and chordality testing — four register allocators spanning the classical and SSA-aware approaches, a RISC-V backend that emits real assembly, a correctness infrastructure, and an evaluation harness.

Out of scope, deliberately: every other optimisation. We do no global value numbering, no loop-invariant code motion, no instruction scheduling. That is not laziness — `clang -O1` has already performed those passes before the IR reaches us, so the code we allocate registers for is genuinely optimised code, and register allocation is cleanly isolated as the only variable. We do no binary encoding; we emit assembly text and let the GNU assembler do its job.

And the last line is the important one. We are not trying to beat LLVM's production allocator. LLVM's `greedy` allocator is the product of many engineer-years. Our target is to land within a constant factor of it and to *explain* the remaining difference. A project that claimed it would beat it would not be credible, and we would rather state a target we can actually defend.

---

# Slide 9 — Proposed Architecture

**On the slide**

```
   C source
      │  clang -O1 -S -emit-llvm
      ▼
  ┌─────────────────────────────────────────────────────────────┐
  │  LLVM IR (.ll)  ──►  PARSER  ──►  Internal SSA IR           │
  └─────────────────────────────────────────────────────────────┘
      │
      ▼
  ┌─────────────────────────────────────────────────────────────┐
  │  ANALYSIS LAYER                                              │
  │  CFG ─► Dominator tree ─► Loop nesting ─► Liveness           │
  │                     └─► Interference graph ─► MCS/chordality │
  └─────────────────────────────────────────────────────────────┘
      │
      ▼
  ┌─────────────────────────────────────────────────────────────┐
  │  REGISTER ALLOCATION   (register budget k is a parameter)   │
  │   ┌────────┐ ┌─────────────┐ ┌───────────────┐ ┌──────────┐ │
  │   │ Naive  │ │ Linear scan │ │ Chaitin–Briggs│ │  Chordal │ │
  │   └────────┘ └─────────────┘ └───────────────┘ └──────────┘ │
  │              └──────►  Iterated register coalescing  ◄─────┘ │
  └─────────────────────────────────────────────────────────────┘
      │
      ▼
  ┌─────────────────────────────────────────────────────────────┐
  │  BACKEND: φ-elimination ─► spill insertion ─► RISC-V ABI     │
  │           ─► textual .s emission                             │
  └─────────────────────────────────────────────────────────────┘
      │                                    │
      ▼                                    ▼
  riscv64-gcc ─► qemu-riscv64        STATIC VERIFIER
      │                              (independent correctness pass)
      ▼
  Execution + instruction counts ──►  EVALUATION HARNESS
                                          ▲
  llc -regalloc={basic,greedy,fast,pbqp} ─┘   (external baselines)
```

> *Redraw this as a proper diagram in the deck — boxes and arrows, colour-coded by layer. Do not paste ASCII art onto a slide.*

**Speaker notes (Section C) — 75 s**

This is the system. I will walk the pipeline top to bottom.

We take C source and run it through `clang -O1` to get textual LLVM IR. Our parser reads that into our own internal SSA representation.

The analysis layer builds the control flow graph, then the dominator tree, then loop nesting depth — which we need for spill cost weighting — then liveness by backward dataflow to a fixpoint, and from liveness, the interference graph. Maximum cardinality search tests that graph for chordality, which is how we answer research question two.

The allocation stage is the centre. Four allocators consume the same interference graph and the same register budget, so they are directly comparable. Crucially, the register count *k* is a parameter, not a constant — that is what makes the pressure sweep free.

The backend then eliminates φ-functions with correct parallel-copy semantics, inserts spill code, applies the RISC-V calling convention, and emits textual assembly. `riscv64-linux-gnu-gcc` assembles and links it, and `qemu-riscv64` runs it.

Two things hang off the side and they are as important as the main line. On the right, a static verifier independently checks every allocation without executing anything. And at the bottom, LLVM's own `llc` compiles the *identical* IR with its four production allocators — so our baselines are not numbers we chose for ourselves.

> **If asked "why LLVM IR and not your own frontend?"** — three reasons, in this order: (1) an unbounded correct test corpus, since any C program that compiles is a test case; (2) the input is guaranteed correct, so every bug we find is unambiguously in our code; (3) it makes LLVM's own allocators available as external baselines on identical input. Reason 3 is the one that matters most.

---

# Slide 10 — Methodology: The Four Allocators

**On the slide**

All four consume the same interference graph and the same register budget *k*. Differences in output are therefore attributable to the algorithm alone.

| Allocator | Method | Role in the study |
|---|---|---|
| **Naive** | Spill everything; load before each use, store after each def | Correctness floor and lower bound. Establishes the pipeline end-to-end before any allocator is smart |
| **Linear scan** (Poletto & Sarkar, 1999) | Live intervals over a linearised block order; O(*n* log *k*) | The speed-oriented baseline; what JIT compilers actually deploy |
| **Chaitin–Briggs** (1981/1994) | Build → coalesce → simplify → optimistic spill → select | **The classical comparison point.** The heuristic whose optimality gap RQ1 measures |
| **SSA / chordal** (Hack & Goos, 2005) | MCS → perfect elimination ordering → greedy colouring along reverse PEO | Provably optimal colouring in linear time *while the graph is chordal* |
| **+ Iterated coalescing** (George & Appel, 1996) | Conservative coalescing interleaved with simplify | Applied as an add-on; measures moves eliminated |

**Implementation:** Python throughout — every algorithm here (dataflow fixpoint, colouring, MCS, coalescing) is compact in Python, and development velocity dominates.

**The consequent design decision:** because Python makes *our* compile times slow, we do not measure allocator cost in wall-clock seconds. We count operations — graph nodes visited, simplify/select iterations, coalescing passes. That metric is machine-independent and constant-factor-free, which makes it a **better** measurement, not a concession.

**Speaker notes (Section C) — 70 s**

Four allocators, and each is there for a specific reason.

The naive allocator spills everything. It is deliberately terrible, and it exists so that we have a correct end-to-end pipeline in week one before any interesting algorithm is written.

Linear scan is the practical baseline — it abandons graph colouring for a single pass over live intervals, and it is what production JIT compilers actually ship.

Chaitin–Briggs is the classical comparison point and the most involved of the four: build the graph, coalesce, simplify by removing low-degree nodes, optimistically push high-degree ones anyway, then assign colours on the way back out. This is the heuristic whose optimality gap our first research question measures.

The chordal allocator is the SSA-aware one. Maximum cardinality search gives a perfect elimination ordering; colouring greedily along its reverse is provably optimal — as long as the graph is still chordal, which is exactly what research question two investigates.

On implementation: we are writing this in Python. Every algorithm involved is short in Python, and for a sixteen-week project development speed dominates execution speed.

That does create one honest problem, and we have designed around it. Python makes our own compiler slow, so wall-clock timing of the allocator would measure our language choice rather than the algorithm. We therefore count operations instead — nodes visited, iterations run. Those numbers are machine-independent and free of constant factors, so they are reproducible by anyone, which we would argue makes them the better metric regardless.

---

# Slide 11 — Evaluation Design

**On the slide**

**Metrics** — all machine-independent and counted, never wall-clock

| Metric | Answers |
|---|---|
| Colours used vs. ω (from the PEO) | **RQ1** — the heuristic-vs-optimal gap |
| Chordality rate per pipeline stage (MCS) | **RQ2** — where and how badly chordality breaks |
| Static spill count (loads/stores inserted) | RQ3, general quality |
| Dynamic spill cost — spills weighted by 10^(loop depth) | RQ3 — the standard proxy for real cost |
| Executed instruction count (QEMU `libinsn` plugin) | End-to-end quality, deterministic |
| Moves remaining after coalescing | Coalescing effectiveness |
| Allocator work — counted operations | Cost side of the quality/cost trade |

**Benchmarks**

- **Polybench/C** — dense loop nests, high register pressure, self-verifying. The most relevant suite for this problem
- **llvm-test-suite SingleSource** — hundreds of small C programs with reference outputs; breadth
- **CoreMark**, **Dhrystone** — recognised standard workloads
- **Csmith** — randomly generated C programs, used for correctness only

**Baselines** — `llc -march=riscv64 -regalloc={basic, greedy, fast, pbqp}` on **identical IR**

> We report distributions across benchmarks — per-program tables plus geometric means — never a single headline number.

**Speaker notes (Section D) — 60 s**

Our evaluation is built around one principle: every number we report should be reproducible by anyone on any machine.

That rules out wall-clock timing, which on a laptop is mostly noise. Instead every metric is a count. Colours used against the true optimum answers research question one. Chordality rate measured at each pipeline stage answers question two. Spill counts — both raw, and weighted by ten to the power of loop nesting depth, which is the standard proxy for dynamic cost — plus executed instruction counts from QEMU's instrumentation plugin give us end-to-end quality.

For benchmarks we use Polybench, which is dense loop nests under genuine register pressure and is the most relevant suite for this problem; the LLVM test suite's single-source programs for breadth; CoreMark and Dhrystone because they are recognised; and Csmith-generated programs for correctness testing.

And the baselines are the part we are most confident about. We run LLVM's own `llc` with all four of its production allocators on the *identical* IR that our compiler receives. That means our comparison point is not a weak baseline we selected for ourselves — it is an industrial allocator on identical input.

Finally, we report distributions, not single numbers. Per-program tables and geometric means, because a single average across a benchmark suite hides exactly the variation we are trying to study.

---

# Slide 12 — Correctness Strategy

**On the slide**

**An allocator that miscompiles silently invalidates every number in the report.** Correctness infrastructure therefore begins in week 1, not at the end.

**Three independent layers**

1. **Static allocation verifier** — a pass independent of the allocator, asserting that no two simultaneously-live values share a physical register, that every use is reached by a definition, and that spill slots do not alias. Catches the majority of bugs without executing anything.
2. **Differential testing against reference output** — every compiled benchmark is executed under QEMU and its stdout diffed against the `clang -O1` build of the same source.
3. **Csmith random program generation** — thousands of randomly generated C programs compiled through our backend, output compared against GCC. This is what turns *"we believe it works"* into *"we have evidence it works."*

**Target for end of Phase 1:** 500+ Csmith-generated programs passing end-to-end.

**Speaker notes (Section D) — 45 s**

One slide on correctness, because it underpins everything else.

Our entire contribution is a set of measurements. If the allocator quietly produces wrong code, every number in our report is worthless. So correctness infrastructure starts in week one, not at the end.

Three independent layers. First, a static verifier — a separate pass that checks the allocation without running anything, asserting that no two simultaneously-live values were given the same register and that spill slots do not overlap. It is cheap to write and it catches most bugs immediately.

Second, differential testing: we run every compiled benchmark and compare its output against the same program compiled by clang.

Third, Csmith — a random C program generator built for exactly this purpose. We generate thousands of programs and compare our output against GCC's. Our target for the end of this phase is five hundred or more Csmith programs passing end-to-end.

---

# Slide 13 — Applications

**On the slide**

**Direct outcomes**

- **Evidence for backend designers.** A quantified answer to whether SSA-aware allocation justifies its engineering cost — currently a decision made on intuition
- **RISC-V embedded toolchains.** RISC-V is the growth target in embedded and academic hardware; register-constrained profiles (RV32E has 16 registers) are exactly the regime where RQ3's pressure sweep is informative
- **JIT compilers.** Linear scan is deployed in JITs precisely because colouring is believed expensive. If optimal colouring on SSA is linear-time, that trade-off deserves re-examination — and our operation counts quantify it

**Reusable artefacts**

- A **readable, documented reference implementation** of four allocators on a common IR — of direct pedagogical value; production allocators are not readable
- The **static verifier and differential-testing harness**, which generalise to any register allocator
- The **interference-graph visualiser**, for teaching and for debugging

**Speaker notes (Section D) — 45 s**

Where this is useful.

First, evidence. Backend engineers currently decide whether to adopt SSA-based allocation largely on intuition. We produce numbers.

Second, RISC-V specifically. RISC-V is where new embedded and academic hardware is going, and its embedded profile RV32E has only sixteen registers. That is precisely the high-pressure regime our third research question sweeps, so the results have a direct audience.

Third, JIT compilers. They use linear scan because graph colouring is assumed too expensive. If optimal colouring on SSA form is linear-time, that assumption is worth revisiting, and our operation counts are exactly the data needed to revisit it.

And beyond the findings there are three reusable artefacts: a readable implementation of four allocators on a shared IR — which has real teaching value, because production allocators are essentially unreadable — a verification harness that generalises to any allocator, and an interference-graph visualiser.

---

# Slide 14 — Work Plan and Current Status

**On the slide**

**Phase 1 — build the system (weeks 1–8).** *Deliverable: a backend compiling real C programs to executing RISC-V binaries.*

| Weeks | Work |
|---|---|
| 1–2 | Toolchain validation · end-to-end spike · `.ll` parser · IR data structures · CFG |
| 3–4 | Dominator tree · loop detection · liveness · interference graph · naive allocator |
| 5–6 | Linear scan allocator · Chaitin–Briggs allocator |
| 7–8 | φ-elimination · RISC-V calling convention · correctness harness · Csmith differential loop |

**Phase 2 — conduct the study (weeks 9–16).** *Deliverable: measured answers, plots, demonstration.*

| Weeks | Work |
|---|---|
| 9–10 | Benchmark integration · `llc` baselines · **chordality survey (RQ2)** |
| 11–12 | **Optimal vs. heuristic colouring (RQ1)** · iterated coalescing |
| 13–14 | **Register budget sweep (RQ3)** · slack |
| 15–16 | Results consolidation · visualiser · report |

**Current status**
- Topic finalised and approved by the guide; scope boundaries fixed
- Core literature reviewed — Hack & Goos and Bouchez et al. worked through in detail
- Architecture and module interfaces specified; work divided across four tracks
- Toolchain validation in progress: `clang -O1 -S -emit-llvm`, `riscv64-linux-gnu-gcc`, `qemu-riscv64`, `llc` baselines

> **Fill this section in with what is actually true tomorrow morning.** Every line must be defensible if the panel asks to see it.

**Speaker notes (Section D) — 45 s**

Our plan splits into two halves of eight weeks.

The first half builds the system, and it is sequenced so that a working backend exists by week eight regardless of how the research questions turn out. Toolchain and parser first, then the analysis layer, then the allocators in increasing order of difficulty, then the backend and the correctness harness.

The second half is the study. Benchmarks and baselines, then the chordality survey, then the optimal-versus-heuristic comparison, then the pressure sweep, then consolidation and the report. Each of our three research questions has specific weeks assigned to it.

We have deliberately structured this so the risk is bounded: even if every research question returns a negative answer, a working SSA-based RISC-V compiler backend with four register allocators exists at the end of week eight, and that is a real deliverable.

Where we are now: the topic is finalised with our guide, the scope is fixed, the core literature is reviewed, the architecture and module interfaces are specified with the work divided into four tracks, and we are validating the toolchain.

---

# Slide 15 — Conclusion and References

**On the slide**

**Conclusion**

- Register allocation is universally taught as NP-hard graph colouring. **For strict SSA form — which every modern compiler uses — that is not true**; colouring is linear-time optimal, and the hardness lies in spilling and coalescing
- We build an SSA-based RISC-V backend with four allocators and measure what that structural result is actually worth in a working compiler
- Three questions: **the heuristic's optimality gap · where chordality breaks under real backend transformations · whether the advantage survives register pressure**
- **Next phase:** complete the analysis layer and the classical allocators, reach a correct end-to-end pipeline validated by Csmith, and begin the chordality survey

**References** *(IEEE format)*

[1] G. J. Chaitin, M. A. Auslander, A. K. Chandra, J. Cocke, M. E. Hopkins, and P. W. Markstein, "Register allocation via coloring," *Computer Languages*, vol. 6, no. 1, pp. 47–57, 1981.

[2] P. Briggs, K. D. Cooper, and L. Torczon, "Improvements to graph coloring register allocation," *ACM Trans. Program. Lang. Syst.*, vol. 16, no. 3, pp. 428–455, May 1994.

[3] L. George and A. W. Appel, "Iterated register coalescing," *ACM Trans. Program. Lang. Syst.*, vol. 18, no. 3, pp. 300–324, May 1996.

[4] M. Poletto and V. Sarkar, "Linear scan register allocation," *ACM Trans. Program. Lang. Syst.*, vol. 21, no. 5, pp. 895–913, Sep. 1999.

[5] S. Hack and G. Goos, "Optimal register allocation for SSA-form programs in polynomial time," *Inf. Process. Lett.*, vol. 98, no. 4, pp. 150–155, May 2006.

[6] F. Bouchez, A. Darte, and F. Rastello, "Register allocation: What does the NP-completeness proof of Chaitin et al. really prove?," in *Proc. 19th Int. Workshop on Languages and Compilers for Parallel Computing (LCPC)*, 2006, pp. 283–298.

[7] R. E. Tarjan and M. Yannakakis, "Simple linear-time algorithms to test chordality of graphs, test acyclicity of hypergraphs, and selectively reduce acyclic hypergraphs," *SIAM J. Comput.*, vol. 13, no. 3, pp. 566–579, Aug. 1984.

[8] F. M. Q. Pereira and J. Palsberg, "Register allocation via coloring of chordal graphs," in *Proc. 3rd Asian Symp. on Programming Languages and Systems (APLAS)*, 2005, pp. 315–329.

**Speaker notes (Section D) — 40 s**

To conclude.

Register allocation is universally taught as NP-hard graph colouring. For strict SSA form — the form every modern compiler already uses — that is not true. Colouring is optimal in linear time, and the real difficulty is in spilling and coalescing. We are building an SSA-based RISC-V backend with four allocators to measure what that structural result is actually worth in a working compiler.

Three questions: how much the classical heuristic loses, where chordality breaks under real backend transformations, and whether any advantage survives register pressure.

For the next review, we expect to have the analysis layer and the classical allocators complete, a correct end-to-end pipeline validated against Csmith, and the chordality survey underway.

Thank you. We are happy to take questions.

---

## Timing budget

| Section | Slides | Target |
|---|---|---|
| A | 1, 2, 4, 5 | 3:30 |
| B | 3, 6, 7 | 3:40 |
| C | 8, 9, 10 | 3:35 |
| D | 11, 12, 13, 14, 15 | 3:15 |
| — | handovers, buffer | 1:00 |
| **Total** | | **15:00** |

**Rehearsal rule:** if the first full run exceeds 15:00, cut *content on slides*, never speech. The most compressible material is the Slide 4 table (drop the "Limitation" column and say it instead) and Slide 11 (drop the two lowest-value metric rows).

---

## Build notes for the .pptx

- Base on `BTP Presentation Template-2-2.pptx` — keep its title layout, the IIIT Kottayam footer, and slide numbering
- Template slide order maps as: Title→1, Introduction→2, Literature Review→4–5, Motivation→6, Problem Statement→7, Architecture→9, Conclusion→15, References→15
- Slides 3, 8, 10, 11, 12, 13, 14 are additions. They are not deviations from the template — they are what the stated evaluation criteria (scope, methodology, applications) require
- **Two figures are worth real effort:** the three-panel CFG→dominator tree→chordal interference graph on Slide 3, and the pipeline diagram on Slide 9. Those two carry the presentation
- Text on slides should be shorter than what is written here. The prose above is the *speaking* content; slides get the skeleton
