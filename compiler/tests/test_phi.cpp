#include <gtest/gtest.h>
#include <map>
#include <random>
#include "ssarv/phielim.hpp"
#include "support/fixtures.hpp"
using namespace ssarv;
TEST(Phi, ParallelCopiesPreserveOldSourcesIncludingCycles) {
    std::mt19937 random(411);
    for (int trial = 0; trial < 100; ++trial) {
        std::vector<std::pair<ValueId,ValueId>> copies; std::map<ValueId,int> before;
        for (ValueId v = 0; v < 8; ++v) { before[v] = 100 + static_cast<int>(v); copies.emplace_back(v, random() % 8); }
        auto after = before; ValueId temp = 8;
        for (auto [dst,src] : sequentialise(copies, [&] { return temp++; })) after[dst] = after.at(src);
        for (auto [dst,src] : copies) EXPECT_EQ(after.at(dst), before.at(src));
    }
}
TEST(Phi, DiamondCopiesAreTypedAndTransformationIsIdempotent) {
    auto fn = fixtures::diamond(); auto report = eliminatePhis(fn);
    EXPECT_EQ(report.phis, 1); EXPECT_EQ(report.criticalEdgesSplit, 0);
    EXPECT_EQ(report.copies, 2);
    for (const auto& b : fn.blocks) for (const auto& i : b.instrs) {
        EXPECT_FALSE(i.isPhi());
        if (i.opcode == "mov") { EXPECT_EQ(i.type, "i32"); }
    }
    EXPECT_EQ(eliminatePhis(fn).copies, 0);
}
TEST(Phi, CriticalSwitchEdgeRetargetsCasesAndPhiPredecessor) {
    Function fn; auto x = fn.values.intern("%x"), p = fn.values.intern("%p"); fn.params = {x};
    fixtures::block(fn, "%entry", {}, {1,2}); fixtures::block(fn, "%other", {0}, {2});
    fixtures::block(fn, "%join", {0,1}, {});
    Instr sw; sw.opcode = "switch"; sw.type = "i32"; sw.uses = {x};
    sw.operands = {fixtures::reg(fn,x)}; sw.succs = {1,2}; sw.cases = {{"7",2,"%join"}};
    fn.blocks[0].instrs = {sw}; fn.blocks[1].instrs = {fixtures::branch({2})};
    Instr phi; phi.opcode = "phi"; phi.type = "i32"; phi.dst = p;
    phi.phiArgs = {{x,"",0,"%entry"},{kNoValue,"9",1,"%other"}}; phi.uses = {x};
    fn.blocks[2].instrs = {phi, fixtures::ret(fn,p)};
    EXPECT_EQ(splitCriticalEdges(fn), 1); ASSERT_EQ(fn.blocks.size(), 4u);
    auto split = fn.blocks[0].instrs.back().cases[0].target; EXPECT_EQ(split, 3u);
    EXPECT_EQ(fn.blocks[2].instrs[0].phiArgs[0].pred, split);
    EXPECT_EQ(fn.blocks[split].preds, (std::vector<BlockId>{0}));
    EXPECT_EQ(fn.blocks[split].succs, (std::vector<BlockId>{2}));
}
