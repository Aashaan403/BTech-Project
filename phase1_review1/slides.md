# BTP Phase 1 — Review 1 — Speaker Notes

**Guide:** Dr. Nandini J Warrier · **Date:** 9 September 2026 · **15 min + 5 min Q&A**
**Graded on:** (a) problem definition and scope · (b) literature survey · (c) architecture/methodology · (d) applications and presentation

> **The deck is `latex/btp-review1.pdf` (15 frames). That is the source of truth.**
> This file is the speaking companion — one section per frame, in order. If the two disagree, the PDF wins.
> Rebuild with `latex/build.sh`.

---

## READ THIS FIRST — what we claim, and what we don't

Learn this paragraph before anything else. Everything else follows from it.

**We are building a compiler. We are not discovering a new theory.**

The theory was settled in 2005–06 by Hack & Goos and by Pereira & Palsberg. We cite them, we agree with them, we don't claim to improve on them. What we build is a RISC-V compiler where the register allocator is a **swappable part**, so five algorithms run on identical input and can be compared fairly. That fair comparison is the point.

**Why it's worth doing:** every published comparison used a *different compiler*, a *different processor*, and *different test programs*. So their numbers can't be compared with each other. Ours can.

**If asked "what's new here?"** — *The theory isn't ours and we don't claim it. What we build is a working RISC-V compiler where five allocators run on identical input, so they can be compared under controlled conditions. Nobody has published that comparison, and nobody has run these algorithms on RISC-V.* Then stop.

**Never say:** "nobody has measured this" · "this is an open problem" · "we'll find out whether the theory holds". All three are false, and the papers on Frames 4–5 disprove them.

---

## Who presents what

| Section | Presenter | Frames | Time |
|---|---|---|---|
| **A** | Rishi Sharma | 1 Title · 2 Introduction · 4 Literature I · 5 Literature II | ~3:15 |
| **B** | **Shaun Alan Joseph** *(lead)* | 3 SSA · 6 Motivation · 7 Problem Statement | ~3:05 |
| **C** | Komal | 8 Scope · 9 Architecture · 10 Five Allocators | ~3:15 |
| **D** | Aswin Parames | 11 Evaluation · 12 Work Plan · 13 Applications · 14 Conclusion | ~3:50 |
| — | handovers + buffer | | ~1:35 |

Shaun takes the conceptual core and anchors Q&A. **Frame 15 (References) is not presented** — leave it up during questions.

**Handover rule:** end with one sentence naming who's next. *"That's the problem we're solving — Komal will take you through the scope and architecture."* Silent swaps read as disorganised.

**D is the longest section.** If the timed run overruns, trim Frames 13 and 14 first — they're the most compressible.

---

# Frame 1 — Title

**Say (Rishi) — 20 s**

Good evening. We're presenting our B.Tech project — *Design and Implementation of an SSA-Based RISC-V Compiler with Comparative Register Allocation* — guided by Dr. Nandini Warrier. Over the next fifteen minutes the four of us will cover the problem, the literature, what we're building, and how we'll evaluate it.

---

# Frame 2 — Introduction: Register Allocation

*On the frame: four bullets — registers vs unlimited values · spilling costs a load and store per access · high-impact backend pass · Chaitin 1981 interference graph.*

**Say (Rishi) — 55 s**

Every program uses far more values than the machine has registers. RISC-V gives us thirty-two; a real function may have hundreds alive at once. Deciding which live in registers and which get pushed to memory is register allocation.

It matters because the cost isn't paid once. A value pushed to memory costs a load and a store *every time it's used* — and if that use is inside a loop, you pay it every iteration.

Chaitin, in 1981, gave the formulation everyone still teaches. Build a graph: each value is a node, and two values get an edge if they're alive at the same time, because then they can't share a register. Now colour that graph, where the colours are your registers.

That reduction — register allocation *is* graph colouring — is the foundation of this whole area.

> **If asked what "alive" means:** a value is alive from where it's computed until its last use. Two values interfere if their alive ranges overlap.

---

# Frame 3 — Background: SSA Form

*On the frame: two columns — what SSA is, and why it changes the problem — plus the chordality figure.*

**Say (Shaun) — 60 s**

A minute on SSA form, because it's what makes our project make sense.

The rule is simple: every value is assigned exactly once. If your program assigns to `x` three times, in SSA you get three separate values. Where two branches come back together you need to say which version survived, and that's what a φ-function does.

This isn't exotic — LLVM, GCC, and the Java and JavaScript JITs all use it. By the time code reaches a modern register allocator it's almost always already in SSA.

Here's why that matters. In general an interference graph can be any shape, and colouring an arbitrary graph is NP-hard — that's the standard reason for using heuristics. But if the program is in SSA form, the graph is always a well-behaved shape called chordal, and chordal graphs colour optimally in linear time. You don't need a heuristic at all.

That was shown by two groups independently in 2005. It's established, not something we're claiming — but it's why one of our five allocators exists.

> **If asked "what is a chordal graph?"** — *"a graph where every cycle of four or more nodes has a shortcut edge across it."* One line, then move on. Don't try to prove anything.
>
> **If asked why SSA gives that shape** — *"in SSA every value is defined once, so the region where it's alive is a connected piece of the dominator tree, and graphs built from overlapping subtrees are exactly the chordal graphs."* If pushed: that's Hack's 2005 result, we're implementing it, not reproving it.

---

# Frame 4 — Literature Survey: Two Lines of Work

*On the frame: two tables — the classical line (Chaitin, Briggs, George & Appel, Poletto & Sarkar) and the SSA line (Tarjan & Yannakakis, Hack & Goos, Pereira & Palsberg, Bouchez et al.).*

**Say (Rishi) — 75 s**

The literature splits into two lines.

The classical line starts with Chaitin in 1981, who gave the interference-graph idea and the hardness result — any graph can be an interference graph, so the problem inherits the NP-hardness of graph colouring. Briggs improved it in 1994 with optimistic colouring, which is what most production allocators descend from. George and Appel added coalescing, which removes redundant copy instructions. Poletto and Sarkar's linear scan gave up graph colouring entirely for speed. All four are heuristics — none promises the best allocation.

The second line is where the interesting result lives. Hack and Goos proved SSA interference graphs are chordal, so colouring them optimally is easy. Pereira and Palsberg built a working chordal allocator and measured it across the entire Java standard library — twenty-three thousand methods, around ninety-five percent chordal. Bouchez and colleagues then pinned down what's actually still hard: not colouring, but deciding what to evict when you run short of registers, and coalescing.

> **If asked "why does the classical line still dominate production compilers?"** — *"engineering inertia and the fact that real backends leave SSA before allocating. That's part of what makes the comparison worth doing."*

---

# Frame 5 — Literature Survey: Their Measured Results

*On the frame: the Pereira & Palsberg Table 1 numbers, plus the "we are not claiming to discover any of this" statement.*

**Say (Rishi) — 45 s**

This table is the important one, and I want to be direct about it.

These researchers didn't just prove things — they measured them. The classical heuristic uses 4.17 registers per method against an optimal 4.13, so the gap is under one percent. Converting to SSA and back actually *increases* the fraction of chordal graphs, from 91 to 95.5 percent. And they compared low and high register counts.

We are not claiming to discover any of this. These are the numbers we expect to reproduce, on a different processor, in our own compiler — and knowing them in advance is how we'll tell whether our implementation is correct.

> **This frame is your armour.** A panel member who knows the field is checking whether you know these results. Showing them first, with numbers, is a strength. **Never claim these questions are open.**

---

# Frame 6 — Motivation: Why Build Another One

*On the frame: the four-row table of published comparisons (different compiler, target, benchmarks each), then what we build instead.*

**Say (Shaun) — 60 s**

So if the theory is settled and the algorithms are published, why build another compiler?

Because the comparisons between them aren't trustworthy. Look at the table. Pereira and Palsberg measured inside a Java compiler on Java bytecode. Hack measured inside libFirm on x86. Poletto and Sarkar used their own infrastructure. LLVM's allocators are only ever compared against each other, inside LLVM.

Each of those is good work. But you can't put their numbers side by side and learn anything — if one reports fewer spills than another, you can't tell whether that's the algorithm, the compiler, the processor, or the test programs.

We're building the controlled version. One compiler, one IR, one benchmark set, one backend, and the allocator is a swappable component. So when the numbers differ, the algorithm is the only thing that changed. And we're doing it on RISC-V, where this comparison hasn't been published.

> **If asked "isn't this just re-running old experiments?"** — *"Partly, and deliberately — reproducing a known result is how we show our compiler is correct. But nobody has run these five algorithms against each other under identical conditions, and nobody has run them on RISC-V at all."*

---

# Frame 7 — Problem Statement

*On the frame: the one-sentence statement, then the three things we measure.*

**Say (Shaun) — 65 s**

Here's the problem statement. We design and implement an SSA-based compiler targeting RISC-V where the register allocator is a swappable component, implement five allocation strategies, and compare them under identical conditions.

Three things we measure.

First, allocation quality — how many values each algorithm is forced to push to memory on the same programs. We compare against LLVM's own production allocators, and for small functions we compute the genuinely optimal answer with an exact solver, so we have a true reference point rather than just algorithms ranked against each other.

Second, the effect of register count, and this is where RISC-V helps us. It has two official configurations — the standard thirty-two registers, and an embedded profile called RV32E with sixteen. Both ship in real hardware. So instead of inventing an artificial register limit, we run everything on both.

Third, the cost of each algorithm. We count operations rather than seconds — nodes visited, iterations run — because counts are machine-independent, so anyone can reproduce our numbers on any hardware.

> **If asked "why not just time it?"** — *"Because seconds depend on the machine, the compiler that built us, and what else is running. Operation counts don't. They're reproducible by anyone, which makes them the better metric for a comparison study."* Do **not** say "because our compiler is slow."

---

# Frame 8 — Scope of Work

*On the frame: the six-row in-scope table (Input, Analysis, Allocation, Backend, Validation, Evaluation), plus the note on LLVM `greedy`.*

**Say (Komal) — 55 s**

This is what we're building.

A parser for LLVM's intermediate representation. A full analysis layer — control flow graph, dominator tree, loop detection, liveness, the interference graph, and a chordality test. Five register allocators. A RISC-V backend that emits real assembly. Correctness infrastructure. And an evaluation harness.

A word on the fifth allocator. Four are practical algorithms. The fifth is an exact solver — slow, so it only runs on small functions, but it gives the genuinely optimal answer to compare the others against. Without it we'd only know how the algorithms rank, not how far any of them is from the best possible.

And the last line matters. We are not trying to beat LLVM's production allocator — that's many engineer-years of work. Our goal is to land within a constant factor of it and explain the gap. A project that claimed it would beat LLVM wouldn't be credible.

> **If asked about the exact solver** — *"we express allocation as an integer linear program and solve it with an off-the-shelf solver. Far too slow for real use, which is why it only runs on small functions, but it gives us ground truth."* Enough for Review 1.
>
> **If asked "do you expect to beat LLVM?"** — *"No, and we don't claim we will. Landing within a constant factor and being able to explain the difference is the target."*

---

# Frame 9 — Proposed Architecture

*On the frame: the full pipeline diagram — C source → clang → .ll → our parser → SSA IR → analysis → **swappable allocator** → backend → RISC-V assembly → gcc → qemu → output check.*

**Say (Komal) — 70 s**

This is the system.

We don't write our own language frontend. We take C source, run it through clang, and get LLVM's intermediate representation as text. Three reasons: any C program that compiles becomes a test case; the input is guaranteed correct, so any bug we find is ours; and most importantly, LLVM's own production allocators run on the exact same file, giving us industrial baselines for free.

From that IR we build the analysis layer — control flow graph, dominator tree, loop nesting, liveness, and the interference graph.

Then the important box. The register allocator is swappable. Everything above and below it is fixed. We pull one algorithm out, drop another in, and nothing else changes. That's what makes our comparison controlled, and it's the central design decision in the project.

Below that, the backend eliminates φ-functions, inserts spill code, applies the RISC-V calling convention, and emits assembly. The GNU RISC-V toolchain assembles and links it, QEMU runs it, and we compare against the same program compiled by clang.

> **If asked "why not write your own frontend?"** — *"it's deliberately borrowed so it isn't a variable in the comparison, and so LLVM's allocators can run on identical input. `llc` is also a compiler that takes IR as input — same shape."*

---

# Frame 10 — The Five Allocators

*On the frame: the six-row table — naive, linear scan, Chaitin–Briggs, SSA/chordal, exact optimum, plus coalescing.*

**Say (Komal) — 70 s**

Five allocators, each there for a reason. All five consume the same interference graph and the same register budget, so any difference in output is caused by the algorithm alone.

The naive allocator spills everything. It's deliberately terrible, and it exists so we have a working pipeline early, before any interesting algorithm is written.

Linear scan skips the graph entirely and makes one pass over the intervals where values are alive. It's what production JITs ship, because it's fast.

Chaitin–Briggs is the classical heuristic and the most involved: build the graph, remove the easy nodes, optimistically push the hard ones anyway, then assign colours on the way back out.

The fourth is the SSA-aware one, and this is where the background pays off. Because SSA interference graphs are chordal, a simple greedy colouring in the right order is provably optimal — not a heuristic, the actual minimum, in linear time.

The fifth is an exact solver. Far too slow for a real compiler, so it only runs on small functions — but it tells us the genuinely best possible answer, so we can say how far each of the other four actually is from optimal.

> **If asked "what language?"** — *"C++. It's the language real compiler backends are written in, and we'll be compiling thousands of benchmark programs across five allocators and two register profiles, so throughput matters."*
>
> **If asked "why five and not two?"** — each is a distinct comparison point: naive is the correctness floor, linear scan is what industry ships, Chaitin–Briggs is the classical heuristic, chordal is the SSA-aware treatment, and the exact solver is ground truth. Removing any one removes a comparison.

---

# Frame 11 — Evaluation and Correctness

*On the frame: two columns — metrics table and the RV32E/RV32I register-count table with baselines — then the three correctness layers.*

**Say (Aswin) — 75 s**

Our evaluation rests on one principle: every number should be reproducible by anyone, on any machine. So every metric is a count, never wall-clock. Spill counts are the headline, weighted by loop depth because a spill inside a nested loop costs far more than one that runs once. Distance from the exact optimum tells us how good the heuristics really are.

The register comparison is specific to RISC-V. Standard profile: thirty-two registers. Embedded profile RV32E: sixteen. Both are official and both ship in real hardware. So rather than inventing an artificial limit, we run everything on both.

The baselines are the part we're most confident about. We run LLVM's own `llc` with all four production allocators on the *identical* IR our compiler receives — so our comparison point isn't a weak baseline we chose ourselves.

And correctness, briefly, because everything depends on it. Our contribution is a set of measurements; if the allocator quietly produces wrong code, every number is worthless. So three independent layers start in week one — a static verifier that checks allocations without executing anything, differential testing against clang's output, and randomly generated programs checked against GCC.

> **Don't state a specific number of test programs.** A number given now is a number you'll be asked about at Review 2.

---

# Frame 12 — Work Plan and Current Status

*On the frame: two columns — Phase 1 weeks 1–8, Phase 2 weeks 9–16 — then the current-status bullets.*

**Say (Aswin) — 65 s**

Our plan splits into two halves of eight weeks. The first builds the compiler, sequenced so a working backend exists by week eight regardless of how the comparison turns out. The second is the comparison itself — benchmarks and baselines, then the chordal allocator, then the exact solver, then the sixteen-versus-thirty-two register comparison, then the report.

The risk is bounded by design. The deliverable is a working compiler with five interchangeable allocators and a controlled comparison between them. That exists whether the numbers turn out interesting or dull — and since we already know roughly what to expect from the literature, dull numbers would tell us our implementation is correct.

Where we are now: title and scope finalised, literature reviewed including the specific results we expect to reproduce, architecture specified with work split four ways.

And the toolchain is validated end to end. A C program compiles through clang to RISC-V IR, through `llc` to assembly, gets assembled and linked, and runs under QEMU producing output identical to the reference build. All four LLVM baseline allocators work on RISC-V — and they already differentiate. On a small loop kernel, the `fast` allocator emits twenty stack accesses where the other three emit two. So the comparison methodology demonstrably works before we've written a line of our own allocator.

> **`verify_toolchain.sh` in the repo reproduces this in ~10 seconds.** Offer to run it live if asked whether you've started.

---

# Frame 13 — Applications

*On the frame: four bullets — the controlled comparison, RISC-V embedded, JIT compilers, reusable artefacts.*

**Say (Aswin) — 45 s**

Where this is useful.

First, the comparison itself — five algorithms, one compiler, one benchmark set, one processor, so for the first time the numbers can be placed side by side and mean something.

Second, RISC-V specifically. The embedded profile has only sixteen registers, which is exactly where allocator choice matters most, and it hasn't been characterised.

Third, JIT compilers, which use linear scan because colouring is assumed too expensive. If optimal colouring on SSA is linear-time, that assumption deserves revisiting, and our operation counts are exactly the data needed.

And beyond the findings, three reusable artefacts: a readable implementation of five allocators on a shared IR, which has real teaching value because production allocators aren't readable; a verification harness that works for any allocator; and an interference-graph visualiser, which is also our demonstration.

> **If asked "how is a C++ implementation more readable than LLVM's?"** — *"LLVM's greedy allocator is tens of thousands of lines entangled with the rest of the backend and tuned for compile time. Ours are a few hundred lines each behind one interface, written to be read. That's a different goal, not a different language."*

---

# Frame 14 — Conclusion

*On the frame: four bullets, then "Thank you — questions welcome."*

**Say (Aswin) — 45 s**

To conclude.

Register allocation is classically graph colouring and NP-hard in general. For SSA form it isn't — optimal colouring is linear-time, and that was established in 2005. We don't claim it.

What's missing is a fair comparison, because every published result comes from a different compiler on a different processor with different benchmarks.

So we're building an SSA-based RISC-V compiler where the allocator is swappable, implementing five strategies, and comparing them under identical conditions across both RISC-V register configurations.

For the next review we expect the analysis layer and the classical allocators complete, a correct end-to-end pipeline, and the comparison underway.

Thank you. We're happy to take questions.

---

# Frame 15 — References

**Not presented.** Leave it up during Q&A so the panel can see the sources while you answer.

---

## Q&A — the six most likely questions

Answer in two or three sentences, then **stop**. Over-answering invites a harder follow-up. Shaun takes anything conceptual; each presenter takes questions on their own frames.

**"What's new here?"**
> The theory isn't ours and we don't claim it — Hack & Goos and Pereira & Palsberg established it in 2005, and we cite both. What we build is a working RISC-V compiler where five allocators run on identical input, so they can be compared under controlled conditions. Nobody has published that comparison, and nobody has run these algorithms on RISC-V.

**"Hasn't Pereira & Palsberg already done this?"**
> They did it inside a Java compiler on Java bytecode, comparing two allocators. We're doing five, on a real machine target with a real calling convention, plus an exact solver so we can say how far from optimal each one is — which they didn't measure. We expect to reproduce their headline numbers, and that's how we'll know our compiler is correct.

**"Why RISC-V rather than x86?"**
> x86 has sixteen registers, register classes, two-address instructions and a complicated calling convention — we'd spend the project on plumbing instead of the allocator. RISC-V has thirty-two uniform registers and a simple ABI. It also has an official sixteen-register embedded profile, which gives us a second data point for free.

**"Why C++?"**
> It's the language real compiler backends are written in, and we'll be compiling thousands of benchmark programs across five allocators and two register profiles, so throughput matters. It also means our allocators sit in the same performance class as the baselines we compare against.

**"Why count operations instead of timing it?"**
> Seconds depend on the machine, the compiler that built us, and what else is running. Operation counts don't — they're reproducible by anyone, which makes them the better metric for a comparison study.

**"What if your results just match what the papers say?"**
> That's the expected outcome and it's the useful one. We know roughly what the published numbers are, so reproducing them is our strongest evidence the compiler is correct. The comparison across five allocators and two register profiles is new regardless of which way it comes out.
