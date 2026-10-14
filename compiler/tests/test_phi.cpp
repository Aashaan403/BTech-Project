#include <gtest/gtest.h>
#include <algorithm>
#include <map>
#include <random>
#include <utility>
#include <vector>
#include "ssarv/phielim.hpp"
#include "support/fixtures.hpp"
#include "support/phi_reachability.hpp"
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
TEST(Phi, ParallelCopiesDiscardIdentityAndBreakLongCyclesOnce) {
    ValueId next = 10;
    const auto moves = sequentialise({{0, 0}, {1, 2}, {2, 3}, {3, 1}},
                                     [&] { return next++; });
    ASSERT_EQ(moves.size(), 4u);
    EXPECT_EQ(moves.front(), (std::pair<ValueId, ValueId>{10, 2}));
    EXPECT_EQ(std::count(moves.begin(), moves.end(), std::pair<ValueId, ValueId>{0, 0}), 0);

    std::map<ValueId, int> before{{0, 100}, {1, 101}, {2, 102}, {3, 103}};
    auto after = before;
    for (const auto& [dst, src] : moves) after[dst] = after.at(src);
    EXPECT_EQ(after[1], before[2]);
    EXPECT_EQ(after[2], before[3]);
    EXPECT_EQ(after[3], before[1]);
}
TEST(Phi, DiamondCopiesAreTypedAndTransformationIsIdempotent) {
    auto fn = fixtures::diamond();
    fixtures::AllReachableFixture reachability(fn);
    auto report = eliminatePhis(fn);
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
    fixtures::AllReachableFixture reachability(fn);
    EXPECT_EQ(splitCriticalEdges(fn), 1); ASSERT_EQ(fn.blocks.size(), 4u);
    auto split = fn.blocks[0].instrs.back().cases[0].target; EXPECT_EQ(split, 3u);
    EXPECT_EQ(fn.blocks[2].instrs[0].phiArgs[0].pred, split);
    EXPECT_EQ(fn.blocks[split].preds, (std::vector<BlockId>{0}));
    EXPECT_EQ(fn.blocks[split].succs, (std::vector<BlockId>{2}));
}
TEST(Phi, MultipleLoopPhisGenerateTypedCycleTemporaryAndLiteralCopies) {
    Function fn;
    const auto c = fn.values.intern("%c");
    const auto x = fn.values.intern("%x");
    const auto y = fn.values.intern("%y");
    fn.params = {c};
    fixtures::block(fn, "%entry", {}, {1});
    fixtures::block(fn, "%join", {0, 2}, {2, 3});
    fixtures::block(fn, "%latch", {1}, {1});
    fixtures::block(fn, "%exit", {1}, {});
    fn.blocks[0].instrs = {fixtures::branch({1})};
    auto choose = fixtures::branch({2, 3});
    choose.uses = {c}; choose.operands = {fixtures::reg(fn, c)};
    fn.blocks[1].instrs = {choose};
    fn.blocks[2].instrs = {fixtures::branch({1})};
    fn.blocks[3].instrs = {fixtures::ret(fn, x)};
    Instr px; px.opcode = "phi"; px.type = "i32"; px.dst = x;
    px.phiArgs = {{kNoValue, "1", 0, "%entry"}, {y, "", 2, "%latch"}};
    px.uses = {y};
    Instr py; py.opcode = "phi"; py.type = "i32"; py.dst = y;
    py.phiArgs = {{kNoValue, "2", 0, "%entry"}, {x, "", 2, "%latch"}};
    py.uses = {x};
    fn.blocks[1].instrs.insert(fn.blocks[1].instrs.begin(), {px, py});

    fixtures::AllReachableFixture reachability(fn);
    const auto report = eliminatePhis(fn);
    EXPECT_EQ(report.phis, 2);
    EXPECT_EQ(report.copies, 5);
    EXPECT_EQ(report.cyclesBroken, 1);
    for (const auto& block : fn.blocks) {
        for (const auto& in : block.instrs) {
            EXPECT_FALSE(in.isPhi());
            if (in.opcode == "mov") EXPECT_EQ(in.type, "i32");
        }
    }
    ASSERT_EQ(fn.blocks[2].instrs.size(), 4u);
    EXPECT_EQ(fn.blocks[2].instrs[0].dst, 3u);  // the fresh temporary
}
