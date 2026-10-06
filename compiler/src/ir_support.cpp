#include "ssarv/ir.hpp"
#include "ssarv/todo.hpp"

namespace ssarv {

ValueId ValuePool::intern(std::string_view) {
    unimplemented("ValuePool::intern");
}

std::optional<ValueId> ValuePool::lookup(std::string_view) const {
    unimplemented("ValuePool::lookup");
}

bool Instr::isTerminator() const {
    unimplemented("Instr::isTerminator");
}

const Instr* Block::terminator() const {
    unimplemented("Block::terminator");
}

std::vector<const Instr*> Block::phis() const {
    unimplemented("Block::phis");
}

std::size_t Function::instrCount() const {
    unimplemented("Function::instrCount");
}

std::vector<ValueId> Function::vregs() const {
    unimplemented("Function::vregs");
}

} // namespace ssarv
