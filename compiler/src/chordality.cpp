#include "ssarv/analysis.hpp"
#include "ssarv/todo.hpp"

namespace ssarv {

std::vector<ValueId> mcsOrder(const Graph&) {
    unimplemented("mcsOrder");
}

bool isPerfectEliminationOrder(const Graph&, const std::vector<ValueId>&) {
    unimplemented("isPerfectEliminationOrder");
}

Chordality isChordal(const Graph&) {
    unimplemented("isChordal");
}

std::size_t cliqueNumberChordal(const Graph&, const std::vector<ValueId>&) {
    unimplemented("cliqueNumberChordal");
}

} // namespace ssarv
