#include "ssarv/phielim.hpp"
#include "ssarv/todo.hpp"

namespace ssarv {

std::vector<std::pair<ValueId, ValueId>> sequentialise(std::vector<std::pair<ValueId, ValueId>>, const std::function<ValueId()>&) {
    unimplemented("sequentialise");
}

int splitCriticalEdges(Function&) {
    unimplemented("splitCriticalEdges");
}

PhiElimStats eliminatePhis(Function&) {
    unimplemented("eliminatePhis");
}

} // namespace ssarv
