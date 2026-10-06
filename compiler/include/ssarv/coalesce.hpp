// Iterated register coalescing (George & Appel 1996), Briggs-conservative.
#pragma once
#include <vector>
#include "ssarv/analysis.hpp"

namespace ssarv {

struct CoalesceReport {
    std::size_t movesTotal = 0;
    std::size_t movesCoalesced = 0;
    std::uint64_t ops = 0;
    std::size_t nodesBefore = 0;
    std::size_t nodesAfter = 0;
};

struct CoalesceResult {
    Graph graph;
    std::vector<ValueId> representative;   ///< union-find root per value
    CoalesceReport report;
};

CoalesceResult coalesce(const Function& fn, const Graph& g, int k,
                        bool conservative = true);

}  // namespace ssarv
