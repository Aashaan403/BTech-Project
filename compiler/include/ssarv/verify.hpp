// Static allocation verifier -- correctness layer 1.
// Independent of every allocator: re-derives interference from the graph
// and asserts the allocation respects it.
#pragma once
#include <string>
#include <vector>
#include "ssarv/allocators.hpp"

namespace ssarv {
std::vector<std::string> verifyAllocation(const Function& fn, const Graph& g,
                                          const AllocResult& r);
}
