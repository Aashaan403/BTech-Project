#include "ssarv/ir.hpp"
#include <algorithm>
#include <array>
#include <stdexcept>

namespace ssarv {
ValueId ValuePool::intern(std::string_view name) {
    std::string key(name);
    if (auto found = index_.find(key); found != index_.end()) return found->second;
    if (names_.size() >= kNoValue) throw std::length_error("Value ID space exhausted");
    auto id = static_cast<ValueId>(names_.size());
    names_.push_back(key);
    try { index_.emplace(std::move(key), id); }
    catch (...) { names_.pop_back(); throw; }
    return id;
}
std::optional<ValueId> ValuePool::lookup(std::string_view name) const {
    auto found = index_.find(std::string(name));
    if (found == index_.end()) return std::nullopt;
    return found->second;
}
bool Instr::isTerminator() const {
    constexpr std::array<std::string_view,10> opcodes = {
        "br", "ret", "switch", "indirectbr", "unreachable", "resume",
        "cleanupret", "catchret", "catchswitch", "callbr"
    };
    return std::ranges::find(opcodes, opcode) != opcodes.end();
}
const Instr* Block::terminator() const {
    if (instrs.empty() || !instrs.back().isTerminator()) return nullptr;
    return &instrs.back();
}
std::vector<const Instr*> Block::phis() const {
    std::vector<const Instr*> found;
    for (const auto& in : instrs) if (in.isPhi()) found.push_back(&in);
    return found;
}
std::size_t Function::instrCount() const {
    std::size_t count = 0;
    for (const auto& b : blocks) count += b.instrs.size();
    return count;
}
std::vector<ValueId> Function::vregs() const {
    auto result = params;
    for (const auto& b : blocks)
        for (const auto& in : b.instrs)
            if (in.dst != kNoValue) result.push_back(in.dst);
    std::ranges::sort(result);
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}
} // namespace ssarv
