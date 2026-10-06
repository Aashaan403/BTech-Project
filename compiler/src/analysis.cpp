#include "ssarv/analysis.hpp"
#include "ssarv/todo.hpp"

namespace ssarv {

std::vector<BlockId> reachableBlocks(const Function&) {
    unimplemented("reachableBlocks");
}

std::vector<BlockId> reversePostorder(const Function&) {
    unimplemented("reversePostorder");
}

std::vector<BlockId> dominators(const Function&) {
    unimplemented("dominators");
}

bool dominates(const std::vector<BlockId>&, BlockId, BlockId) {
    unimplemented("dominates");
}

std::vector<int> loopDepths(const Function&) {
    unimplemented("loopDepths");
}

Liveness computeLiveness(const Function&) {
    unimplemented("computeLiveness");
}

Interference interferenceGraph(const Function&) {
    unimplemented("interferenceGraph");
}

} // namespace ssarv
