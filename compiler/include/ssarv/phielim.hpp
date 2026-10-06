// phi-elimination with correct parallel-copy semantics.
#pragma once
#include <functional>
#include <string>
#include <utility>
#include <vector>
#include "ssarv/ir.hpp"

namespace ssarv {

struct PhiElimStats {
    int criticalEdgesSplit = 0;
    int phis = 0;
    int copies = 0;
    int cyclesBroken = 0;
};

/// Order a parallel assignment {dst <- src} into sequential moves,
/// breaking cycles with a fresh temporary from `makeTemp`.
std::vector<std::pair<ValueId, ValueId>> sequentialise(
    std::vector<std::pair<ValueId, ValueId>> pairs,
    const std::function<ValueId()>& makeTemp);

int splitCriticalEdges(Function& fn);
PhiElimStats eliminatePhis(Function& fn);

}  // namespace ssarv
