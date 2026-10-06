#include <gtest/gtest.h>
#include <algorithm>
#include "support/fixtures.hpp"
using namespace ssarv;
TEST(Analysis, EmptyFunction) {
    Function fn;
    EXPECT_TRUE(reachableBlocks(fn).empty()); EXPECT_TRUE(reversePostorder(fn).empty());
    EXPECT_TRUE(dominators(fn).empty()); EXPECT_TRUE(loopDepths(fn).empty());
    EXPECT_TRUE(computeLiveness(fn).liveIn.empty());
    auto inf = interferenceGraph(fn); EXPECT_EQ(inf.maxlive, 0u); EXPECT_EQ(inf.graph.numNodes(), 0u);
}
TEST(Analysis, DiamondDominatorsTraversalAndPhiEdges) {
    auto fn = fixtures::diamond();
    auto reach = reachableBlocks(fn), rpo = reversePostorder(fn);
    ASSERT_EQ(reach.size(), 4u); ASSERT_EQ(rpo.size(), 4u);
    EXPECT_EQ(reach.front(), 0u); EXPECT_EQ(rpo.front(), 0u);
    std::sort(reach.begin(), reach.end()); EXPECT_EQ(reach, (std::vector<BlockId>{0,1,2,3}));
    EXPECT_EQ(dominators(fn), (std::vector<BlockId>{0,0,0,0}));
    auto lv = computeLiveness(fn);
    ASSERT_EQ(lv.liveIn.size(), 4u); ASSERT_EQ(lv.liveOut.size(), 4u);
    EXPECT_EQ(lv.liveIn[0].toVector(), (std::vector<ValueId>{0,1}));
    EXPECT_EQ(lv.liveOut[1].toVector(), (std::vector<ValueId>{2}));
    EXPECT_EQ(lv.liveOut[2].toVector(), (std::vector<ValueId>{3}));
    EXPECT_EQ(lv.liveIn[3].toVector(), (std::vector<ValueId>{4}));
    auto inf = interferenceGraph(fn);
    EXPECT_EQ(inf.graph.nodes(), (std::vector<ValueId>{0,1,2,3,4}));
    EXPECT_TRUE(inf.graph.hasEdge(0,1)); EXPECT_EQ(inf.graph.numEdges(), 1u);
    EXPECT_EQ(inf.maxlive, 2u);
}
TEST(Analysis, UnreachableBlocksAndInvalidDominanceQueries) {
    auto fn = fixtures::straight(); fixtures::block(fn, "%dead", {}, {});
    auto dead = fn.values.intern("%deadvalue"); Instr i; i.opcode = "add"; i.dst = dead;
    i.type = "i32"; i.uses = {0}; fn.blocks[1].instrs = {i, fixtures::ret(fn, dead)};
    EXPECT_EQ(reachableBlocks(fn), (std::vector<BlockId>{0}));
    auto idom = dominators(fn); ASSERT_EQ(idom.size(), 2u);
    EXPECT_EQ(idom[1], kNoBlock); EXPECT_FALSE(dominates(idom, 1, 1));
    EXPECT_FALSE(dominates(idom, kNoBlock, 0)); EXPECT_TRUE(dominates(idom, 0, 0));
    auto lv = computeLiveness(fn); EXPECT_TRUE(lv.liveIn[1].empty()); EXPECT_TRUE(lv.liveOut[1].empty());
    EXPECT_EQ(interferenceGraph(fn).graph.degree(dead), 0u);
}
TEST(Analysis, TwoLatchesCountAsOneNaturalLoop) {
    auto fn = fixtures::twoLatchLoop();
    EXPECT_EQ(loopDepths(fn), (std::vector<int>{0,1,1,1}));
}
