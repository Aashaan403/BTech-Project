#pragma once
#include "ssarv/analysis.hpp"
#include <initializer_list>

namespace fixtures {
using namespace ssarv;
inline Operand reg(const Function& fn, ValueId v) { return {v, fn.values.name(v)}; }
inline Instr ret(const Function& fn, ValueId v) {
    Instr in; in.opcode = "ret"; in.type = "i32";
    in.uses = {v}; in.operands = {reg(fn, v)}; return in;
}
inline Instr branch(std::initializer_list<BlockId> targets) {
    Instr in; in.opcode = "br"; in.succs = targets; return in;
}
inline void block(Function& fn, const char* label,
                  std::initializer_list<BlockId> preds,
                  std::initializer_list<BlockId> succs) {
    Block b; b.id = static_cast<BlockId>(fn.blocks.size()); b.label = label;
    b.preds = preds; b.succs = succs;
    fn.labelToBlock.emplace(b.label, b.id); fn.blocks.push_back(b);
}
inline Function straight() {
    Function fn; fn.name = "straight";
    auto x = fn.values.intern("%x"), y = fn.values.intern("%y");
    fn.params = {x}; block(fn, "%entry", {}, {});
    Instr add; add.opcode = "add"; add.type = "i32"; add.dst = y;
    add.uses = {x}; add.operands = {reg(fn, x), reg(fn, x)};
    fn.blocks[0].instrs = {add, ret(fn, y)}; return fn;
}
inline Function diamond() {
    Function fn; fn.name = "diamond";
    auto c = fn.values.intern("%c"), x = fn.values.intern("%x");
    auto a = fn.values.intern("%a"), b = fn.values.intern("%b"), p = fn.values.intern("%p");
    fn.params = {c, x};
    block(fn, "%entry", {}, {1, 2}); block(fn, "%left", {0}, {3});
    block(fn, "%right", {0}, {3}); block(fn, "%join", {1, 2}, {});
    auto br = branch({1, 2}); br.uses = {c}; br.operands = {reg(fn, c)};
    fn.blocks[0].instrs = {br};
    for (BlockId id : {1u, 2u}) {
        Instr in; in.opcode = id == 1 ? "add" : "sub";
        in.type = "i32"; in.dst = id == 1 ? a : b;
        in.uses = {x}; in.operands = {reg(fn, x), {kNoValue, "1"}};
        fn.blocks[id].instrs = {in, branch({3})};
    }
    Instr phi; phi.opcode = "phi"; phi.type = "i32"; phi.dst = p;
    phi.uses = {a, b}; phi.phiArgs = {{a, "", 1, "%left"}, {b, "", 2, "%right"}};
    fn.blocks[3].instrs = {phi, ret(fn, p)}; return fn;
}
inline Function twoLatchLoop() {
    Function fn; fn.name = "loop"; auto c = fn.values.intern("%c"); fn.params = {c};
    block(fn, "%entry", {}, {1}); block(fn, "%header", {0, 2, 3}, {2, 3});
    block(fn, "%left", {1}, {1}); block(fn, "%right", {1}, {1});
    fn.blocks[0].instrs = {branch({1})};
    auto cond = branch({2, 3}); cond.uses = {c}; cond.operands = {reg(fn, c)};
    fn.blocks[1].instrs = {cond}; fn.blocks[2].instrs = {branch({1})};
    fn.blocks[3].instrs = {branch({1})}; return fn;
}
inline BitSet bits(std::size_t n, std::initializer_list<ValueId> values) {
    BitSet b(n); for (auto v : values) b.set(v); return b;
}
// Manually derived fixture: conservative parameter edge c--x, no other edges.
// No borrowed pool pointer is retained across a returned Function move.
inline Graph diamondGraph(const Function& fn) {
    Graph g(fn.numValues()); for (ValueId v = 0; v < fn.numValues(); ++v) g.addNode(v);
    g.addEdge(0, 1); return g;
}
} // namespace fixtures
