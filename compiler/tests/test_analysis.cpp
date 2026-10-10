#include <gtest/gtest.h>
#include <algorithm>
#include "support/fixtures.hpp"

using namespace ssarv;

TEST(Analysis, EmptyFunction) {
    Function fn;
    EXPECT_TRUE(reachableBlocks(fn).empty());
    EXPECT_TRUE(reversePostorder(fn).empty());
    EXPECT_TRUE(dominators(fn).empty());
    EXPECT_TRUE(loopDepths(fn).empty());
    EXPECT_TRUE(computeLiveness(fn).liveIn.empty());
    auto inf = interferenceGraph(fn);
    EXPECT_EQ(inf.maxlive, 0u);
    EXPECT_EQ(inf.graph.numNodes(), 0u);
}

TEST(Analysis, DiamondDominatorsTraversalAndPhiEdges) {
    auto fn = fixtures::diamond();
    auto reach = reachableBlocks(fn), rpo = reversePostorder(fn);
    ASSERT_EQ(reach.size(), 4u);
    ASSERT_EQ(rpo.size(), 4u);
    EXPECT_EQ(reach.front(), 0u);
    EXPECT_EQ(rpo.front(), 0u);
    std::sort(reach.begin(), reach.end());
    EXPECT_EQ(reach, (std::vector<BlockId>{0, 1, 2, 3}));
    EXPECT_EQ(dominators(fn), (std::vector<BlockId>{0, 0, 0, 0}));

    auto lv = computeLiveness(fn);
    ASSERT_EQ(lv.liveIn.size(), 4u);
    ASSERT_EQ(lv.liveOut.size(), 4u);
    EXPECT_EQ(lv.liveIn[0].toVector(), (std::vector<ValueId>{0, 1}));
    EXPECT_EQ(lv.liveOut[1].toVector(), (std::vector<ValueId>{2}));
    EXPECT_EQ(lv.liveOut[2].toVector(), (std::vector<ValueId>{3}));
    EXPECT_EQ(lv.liveIn[3].toVector(), (std::vector<ValueId>{4}));

    auto inf = interferenceGraph(fn);
    EXPECT_EQ(inf.graph.nodes(), (std::vector<ValueId>{0, 1, 2, 3, 4}));
    EXPECT_TRUE(inf.graph.hasEdge(0, 1));
    EXPECT_EQ(inf.graph.numEdges(), 1u);
    EXPECT_EQ(inf.maxlive, 2u);
}

TEST(Analysis, UnreachableBlocksAndInvalidDominanceQueries) {
    auto fn = fixtures::straight();
    fixtures::block(fn, "%dead", {}, {});
    auto dead = fn.values.intern("%deadvalue");
    Instr i;
    i.opcode = "add";
    i.dst = dead;
    i.type = "i32";
    i.uses = {0};
    fn.blocks[1].instrs = {i, fixtures::ret(fn, dead)};

    EXPECT_EQ(reachableBlocks(fn), (std::vector<BlockId>{0}));
    auto idom = dominators(fn);
    ASSERT_EQ(idom.size(), 2u);
    EXPECT_EQ(idom[1], kNoBlock);
    EXPECT_FALSE(dominates(idom, 1, 1));
    EXPECT_FALSE(dominates(idom, kNoBlock, 0));
    EXPECT_TRUE(dominates(idom, 0, 0));

    auto lv = computeLiveness(fn);
    EXPECT_TRUE(lv.liveIn[1].empty());
    EXPECT_TRUE(lv.liveOut[1].empty());
    EXPECT_EQ(interferenceGraph(fn).graph.degree(dead), 0u);
}

TEST(Analysis, TwoLatchesCountAsOneNaturalLoop) {
    auto fn = fixtures::twoLatchLoop();
    EXPECT_EQ(loopDepths(fn), (std::vector<int>{0, 1, 1, 1}));
}

TEST(Analysis, NestedLoopsDepthCalculation) {
    // 0: entry -> 1 (outer header)
    // 1: outer header -> 2 (inner header), 5 (exit)
    // 2: inner header -> 3 (inner body), 4 (outer latch)
    // 3: inner body -> 2 (inner latch)
    // 4: outer latch -> 1 (outer latch)
    // 5: exit
    Function fn;
    fn.name = "nested";
    fixtures::block(fn, "%entry", {}, {1});
    fixtures::block(fn, "%outer_hdr", {0, 4}, {2, 5});
    fixtures::block(fn, "%inner_hdr", {1, 3}, {3, 4});
    fixtures::block(fn, "%inner_body", {2}, {2});
    fixtures::block(fn, "%outer_latch", {2}, {1});
    fixtures::block(fn, "%exit", {1}, {});

    fn.blocks[0].instrs = {fixtures::branch({1})};
    fn.blocks[1].instrs = {fixtures::branch({2, 5})};
    fn.blocks[2].instrs = {fixtures::branch({3, 4})};
    fn.blocks[3].instrs = {fixtures::branch({2})};
    fn.blocks[4].instrs = {fixtures::branch({1})};
    auto x = fn.values.intern("%ret");
    fn.blocks[5].instrs = {fixtures::ret(fn, x)};

    auto idom = dominators(fn);
    EXPECT_EQ(idom[0], 0u);
    EXPECT_EQ(idom[1], 0u);
    EXPECT_EQ(idom[2], 1u);
    EXPECT_EQ(idom[3], 2u);
    EXPECT_EQ(idom[4], 2u);
    EXPECT_EQ(idom[5], 1u);

    // depths: 0 -> 0, 1 -> 1, 2 -> 2, 3 -> 2, 4 -> 1, 5 -> 0
    EXPECT_EQ(loopDepths(fn), (std::vector<int>{0, 1, 2, 2, 1, 0}));
}

TEST(Analysis, MultiplePhisInJoinBlockLiveInterferenceAndDeadPhi) {
    // Left and right define a1, a2, a3 and b1, b2, b3
    // Join block has:
    //   p1 = phi [a1, left], [b1, right] (live: used in add)
    //   p2 = phi [a2, left], [b2, right] (live: used in add)
    //   p3 = phi [a3, left], [b3, right] (dead: never used)
    //   res = add p1, p2
    //   ret res
    Function fn;
    fn.name = "multi_phi";
    auto c = fn.values.intern("%c");
    auto a1 = fn.values.intern("%a1"), a2 = fn.values.intern("%a2"), a3 = fn.values.intern("%a3");
    auto b1 = fn.values.intern("%b1"), b2 = fn.values.intern("%b2"), b3 = fn.values.intern("%b3");
    auto p1 = fn.values.intern("%p1"), p2 = fn.values.intern("%p2"), p3 = fn.values.intern("%p3");
    auto res = fn.values.intern("%res");

    fn.params = {c};
    fixtures::block(fn, "%entry", {}, {1, 2});
    fixtures::block(fn, "%left", {0}, {3});
    fixtures::block(fn, "%right", {0}, {3});
    fixtures::block(fn, "%join", {1, 2}, {});

    auto br = fixtures::branch({1, 2});
    br.uses = {c};
    br.operands = {fixtures::reg(fn, c)};
    fn.blocks[0].instrs = {br};

    auto makeDef = [&](ValueId dst) {
        Instr in; in.opcode = "add"; in.type = "i32"; in.dst = dst;
        in.uses = {}; in.operands = {{kNoValue, "1"}, {kNoValue, "2"}};
        return in;
    };

    fn.blocks[1].instrs = {makeDef(a1), makeDef(a2), makeDef(a3), fixtures::branch({3})};
    fn.blocks[2].instrs = {makeDef(b1), makeDef(b2), makeDef(b3), fixtures::branch({3})};

    Instr phi1; phi1.opcode = "phi"; phi1.type = "i32"; phi1.dst = p1;
    phi1.phiArgs = {{a1, "", 1, "%left"}, {b1, "", 2, "%right"}}; phi1.uses = {a1, b1};

    Instr phi2; phi2.opcode = "phi"; phi2.type = "i32"; phi2.dst = p2;
    phi2.phiArgs = {{a2, "", 1, "%left"}, {b2, "", 2, "%right"}}; phi2.uses = {a2, b2};

    Instr phi3; phi3.opcode = "phi"; phi3.type = "i32"; phi3.dst = p3;
    phi3.phiArgs = {{a3, "", 1, "%left"}, {b3, "", 2, "%right"}}; phi3.uses = {a3, b3};

    Instr add; add.opcode = "add"; add.type = "i32"; add.dst = res;
    add.uses = {p1, p2}; add.operands = {fixtures::reg(fn, p1), fixtures::reg(fn, p2)};

    fn.blocks[3].instrs = {phi1, phi2, phi3, add, fixtures::ret(fn, res)};

    auto lv = computeLiveness(fn);
    // In join block (3), liveIn includes all phi destinations p1, p2, p3
    EXPECT_TRUE(lv.liveIn[3].test(p1));
    EXPECT_TRUE(lv.liveIn[3].test(p2));
    EXPECT_TRUE(lv.liveIn[3].test(p3));

    auto inf = interferenceGraph(fn);
    // p1 and p2 are both live at the phi boundary, so they conflict!
    EXPECT_TRUE(inf.graph.hasEdge(p1, p2));
    // p3 is dead at the phi boundary, so it does NOT conflict with p1 or p2!
    EXPECT_FALSE(inf.graph.hasEdge(p1, p3));
    EXPECT_FALSE(inf.graph.hasEdge(p2, p3));
}

TEST(Analysis, UnreachableCycleRemainsIsolated) {
    // 0 -> 1 -> ret
    // 2 <-> 3 (unreachable cycle)
    Function fn;
    fn.name = "unreachable_cycle";
    auto x = fn.values.intern("%x");
    auto d1 = fn.values.intern("%d1");
    auto d2 = fn.values.intern("%d2");
    fn.params = {x};

    fixtures::block(fn, "%entry", {}, {1});
    fixtures::block(fn, "%exit", {0}, {});
    fixtures::block(fn, "%dead1", {3}, {3});
    fixtures::block(fn, "%dead2", {2}, {2});

    fn.blocks[0].instrs = {fixtures::branch({1})};
    fn.blocks[1].instrs = {fixtures::ret(fn, x)};

    Instr deadAdd1; deadAdd1.opcode = "add"; deadAdd1.type = "i32"; deadAdd1.dst = d1;
    deadAdd1.uses = {d2}; deadAdd1.operands = {fixtures::reg(fn, d2)};
    fn.blocks[2].instrs = {deadAdd1, fixtures::branch({3})};

    Instr deadAdd2; deadAdd2.opcode = "add"; deadAdd2.type = "i32"; deadAdd2.dst = d2;
    deadAdd2.uses = {d1}; deadAdd2.operands = {fixtures::reg(fn, d1)};
    fn.blocks[3].instrs = {deadAdd2, fixtures::branch({2})};

    EXPECT_EQ(reachableBlocks(fn), (std::vector<BlockId>{0, 1}));
    auto idom = dominators(fn);
    EXPECT_EQ(idom[0], 0u);
    EXPECT_EQ(idom[1], 0u);
    EXPECT_EQ(idom[2], kNoBlock);
    EXPECT_EQ(idom[3], kNoBlock);

    EXPECT_EQ(loopDepths(fn), (std::vector<int>{0, 0, 0, 0}));

    auto lv = computeLiveness(fn);
    EXPECT_TRUE(lv.liveIn[2].empty());
    EXPECT_TRUE(lv.liveOut[2].empty());
    EXPECT_TRUE(lv.liveIn[3].empty());
    EXPECT_TRUE(lv.liveOut[3].empty());

    auto inf = interferenceGraph(fn);
    EXPECT_EQ(inf.graph.degree(d1), 0u);
    EXPECT_EQ(inf.graph.degree(d2), 0u);
}

TEST(Analysis, ManuallyVerifiedInterferenceEdgesAndMaxLive) {
    // Straight line with 2 parameters: %p0, %p1
    // %t1 = add %p0, 1      (%p0 dies, %p1 live, %t1 live) -> (%t1, %p1) conflict
    // %t2 = add %p1, 2      (%p1 dies, %t1 live, %t2 live) -> (%t2, %t1) conflict
    // %t3 = add %t1, %t2    (%t1, %t2 die, %t3 live)
    // ret %t3
    Function fn;
    fn.name = "precise_liveness";
    auto p0 = fn.values.intern("%p0");
    auto p1 = fn.values.intern("%p1");
    auto t1 = fn.values.intern("%t1");
    auto t2 = fn.values.intern("%t2");
    auto t3 = fn.values.intern("%t3");
    fn.params = {p0, p1};

    fixtures::block(fn, "%entry", {}, {});

    Instr i1; i1.opcode = "add"; i1.type = "i32"; i1.dst = t1;
    i1.uses = {p0}; i1.operands = {fixtures::reg(fn, p0), {kNoValue, "1"}};

    Instr i2; i2.opcode = "add"; i2.type = "i32"; i2.dst = t2;
    i2.uses = {p1}; i2.operands = {fixtures::reg(fn, p1), {kNoValue, "2"}};

    Instr i3; i3.opcode = "add"; i3.type = "i32"; i3.dst = t3;
    i3.uses = {t1, t2}; i3.operands = {fixtures::reg(fn, t1), fixtures::reg(fn, t2)};

    fn.blocks[0].instrs = {i1, i2, i3, fixtures::ret(fn, t3)};

    auto lv = computeLiveness(fn);
    EXPECT_EQ(lv.liveIn[0].toVector(), (std::vector<ValueId>{p0, p1}));
    EXPECT_TRUE(lv.liveOut[0].empty());

    auto inf = interferenceGraph(fn);
    // Parameters conflict conservatively: (p0, p1)
    EXPECT_TRUE(inf.graph.hasEdge(p0, p1));
    // p1 is live when t1 is defined: (t1, p1)
    EXPECT_TRUE(inf.graph.hasEdge(t1, p1));
    // t1 is live when t2 is defined: (t1, t2)
    EXPECT_TRUE(inf.graph.hasEdge(t1, t2));
    // p0 dies at i1, so it does not conflict with t1, t2, or t3
    EXPECT_FALSE(inf.graph.hasEdge(p0, t1));
    EXPECT_FALSE(inf.graph.hasEdge(p0, t2));
    EXPECT_FALSE(inf.graph.hasEdge(p0, t3));
    // t3 is defined when t1 and t2 die, so t3 does not conflict with t1 or t2
    EXPECT_FALSE(inf.graph.hasEdge(t3, t1));
    EXPECT_FALSE(inf.graph.hasEdge(t3, t2));

    EXPECT_EQ(inf.graph.numEdges(), 3u);
    EXPECT_EQ(inf.maxlive, 2u);
}
