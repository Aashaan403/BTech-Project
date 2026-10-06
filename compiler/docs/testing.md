# Independent testing

Configure and build as shown in the README. GoogleTest is required for tests;
`-DBUILD_TESTING=OFF` builds the component libraries and executable alone.

| Target | Dependencies | CMake switch |
|---|---|---|
| `utilities-tests` | Group 4 shared utilities | `SSARV_TEST_UTILITIES` |
| `analysis-tests` | Group 1 + shared utilities | `SSARV_TEST_ANALYSIS` |
| `allocators-tests` | Group 2 + fixture provider + utilities | `SSARV_TEST_ALLOCATORS` |
| `phi-tests` | Group 3 phi + declared reachability + utilities | `SSARV_TEST_PHI` |
| `lowering-tests` | Group 3 lowering + recording context | `SSARV_TEST_LOWERING` |
| `lead-tests` | Group 4 parser/chordality/verifier + utilities | `SSARV_TEST_LEAD` |
| `integration-tests` | All production components | `SSARV_TEST_INTEGRATION` |

During development, run a binary directly, for example:

```sh
cmake --build build --target allocators-tests -j 4
./build/allocators-tests
```

Enable completed component checks for CTest explicitly:

```sh
cmake -S . -B build -DSSARV_TEST_ALLOCATORS=ON
ctest --test-dir build -L group2 --output-on-failure
```

Group 3 enables `SSARV_TEST_PHI` and `SSARV_TEST_LOWERING`. Enable both checks
before considering the group complete. Enable every switch for final acceptance.
Utility checks are enabled by default after milestone 2.
Default green checks cover only the current completed milestone; disabled
checks are pending work. Do not report them as passed.

## Fixture boundary

`tests/support/analysis_provider.cpp` exposes ONLY stored reachable order,
reverse postorder, depth and liveness fixtures to the allocator test binary.
Tests register expected metadata for each Function using `AnalysisFixture`.
Unknown functions throw; no CFG traversal, dominator or liveness algorithm is
implemented there. Never link this provider into the compiler or integration
checks. New allocator fixtures require manually specified analysis results.

Phi fixtures specify resolved CFG IDs and links explicitly.
`phi_reachability.cpp` returns dense block IDs only for functions registered
with `AllReachableFixture`: these controlled examples declare every block
reachable, including new critical-edge blocks. Unknown functions throw.
This is a test assumption, not a reachability implementation; do not use it
for fixtures with unreachable blocks or link it into production. Lowering tests
record operand requests, emitted text and result commits; they do not execute
RISC-V. Integration must add execution checks to validate arithmetic widths,
signedness, calls, spills and aliasing on a real assembler/emulator.

Initial tests are a baseline. Extend coverage for the edge cases listed in each
task and in the shared interface contract.
