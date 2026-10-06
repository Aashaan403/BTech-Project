#include "ssarv/allocators.hpp"
#include "ssarv/todo.hpp"

namespace ssarv {

AllocResult allocChordal(const Function&, const Graph&, int) {
    unimplemented("allocChordal");
}

AllocResult allocChordalBelady(const Function&, const Graph&, int) {
    unimplemented("allocChordalBelady");
}

AllocResult allocChordalCost(const Function&, const Graph&, int) {
    unimplemented("allocChordalCost");
}

AllocResult allocOptimal(const Function&, const Graph&, int, std::size_t) {
    unimplemented("allocOptimal");
}

ExactResult exactChromatic(const Graph&, std::size_t, std::uint64_t) {
    unimplemented("exactChromatic");
}

const std::vector<AllocatorEntry>& allAllocators() {
    unimplemented("allAllocators");
}

} // namespace ssarv
