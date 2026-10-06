#include "ssarv/backend.hpp"
#include "ssarv/todo.hpp"
namespace ssarv {
// ABI colour order is part of the shared interface, not an allocator.
const char* const kRegNames[] = {
    "t0", "t1", "t2", "t3", "t4", "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11"
};
bool isEmittable(const Function&, std::vector<std::string>*) {
    unimplemented("isEmittable");
}
EmitResult emitFunction(const Function&, const AllocResult&) {
    unimplemented("emitFunction");
}
} // namespace ssarv
