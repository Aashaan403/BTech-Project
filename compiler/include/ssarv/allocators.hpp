// Register allocation strategies over a common interference graph.
//
// Every allocator takes the same (function, graph, k) so that any
// difference in output is attributable to the algorithm alone.  Work is
// reported as counted operations, not wall-clock time.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "ssarv/analysis.hpp"

namespace ssarv {

inline constexpr int kSpill = -1;
inline constexpr int kUnassigned = -2;

struct AllocResult {
    std::string name;
    int k = 0;
    std::vector<int> assignment;    ///< indexed by ValueId
    std::vector<ValueId> spilled;
    std::uint64_t ops = 0;
    bool exact = false;              ///< was optimality actually proven?
    std::string note;

    int coloursUsed() const;
    std::size_t numSpills() const { return spilled.size(); }
    int colourOf(ValueId v) const {
        return v < assignment.size() ? assignment[v] : kUnassigned;
    }
};

/// Static spill cost: every def/use weighted by 10^(loop nesting depth).
std::vector<double> spillCosts(const Function& fn);

AllocResult allocNaive(const Function& fn, const Graph& g, int k);
AllocResult allocLinearScan(const Function& fn, const Graph& g, int k);
AllocResult allocChaitinBriggs(const Function& fn, const Graph& g, int k);
AllocResult allocChordal(const Function& fn, const Graph& g, int k);
/// Chordal colouring preceded by a Belady-style spill phase (Hack's method:
/// spill until MAXLIVE <= k, then colour optimally).
AllocResult allocChordalBelady(const Function& fn, const Graph& g, int k);
/// Chordal colouring preceded by a *cost-aware* spill phase: evict the
/// value with the lowest loop-weighted spill cost rather than the one whose
/// next use is furthest away.
AllocResult allocChordalCost(const Function& fn, const Graph& g, int k);
AllocResult allocOptimal(const Function& fn, const Graph& g, int k,
                         std::size_t limitNodes = 60);
/// Exact allocation *including spilling*, as an integer linear program.
/// Requires HiGHS; without it this falls back to allocOptimal.
AllocResult allocOptimalILP(const Function& fn, const Graph& g, int k);

/// Exact chromatic number by DSATUR branch and bound.
/// Returns {chi, nodesExplored}; chi == 0 means "not attempted".
struct ExactResult {
    std::size_t chi = 0;
    std::uint64_t explored = 0;
    bool solved = false;
};
ExactResult exactChromatic(const Graph& g, std::size_t limitNodes = 60,
                           std::uint64_t nodeBudget = 200000);

struct AllocatorEntry {
    const char* name;
    AllocResult (*fn)(const Function&, const Graph&, int);
};
const std::vector<AllocatorEntry>& allAllocators();

}  // namespace ssarv
