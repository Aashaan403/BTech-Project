#include "ssarv/allocators.hpp"
#include "ssarv/todo.hpp"

namespace ssarv {

std::vector<double> spillCosts(const Function&) {
    unimplemented("spillCosts");
}

AllocResult allocNaive(const Function&, const Graph&, int) {
    unimplemented("allocNaive");
}

AllocResult allocLinearScan(const Function&, const Graph&, int) {
    unimplemented("allocLinearScan");
}

AllocResult allocChaitinBriggs(const Function&, const Graph&, int) {
    unimplemented("allocChaitinBriggs");
}

} // namespace ssarv
