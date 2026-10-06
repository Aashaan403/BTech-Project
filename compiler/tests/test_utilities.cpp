#include <gtest/gtest.h>
#include <stdexcept>
#include "support/fixtures.hpp"
#include "ssarv/allocators.hpp"
using namespace ssarv;

TEST(Values, StableDenseInterningAndLookup) {
    ValuePool pool;
    EXPECT_FALSE(pool.lookup("%missing"));
    EXPECT_EQ(pool.size(), 0u);
    EXPECT_EQ(pool.intern("%z"), 0u);
    EXPECT_EQ(pool.intern("%a"), 1u);
    EXPECT_EQ(pool.intern("%z"), 0u);
    EXPECT_EQ(pool.lookup("%a"), std::optional<ValueId>{1});
    EXPECT_EQ(pool.name(0), "%z"); EXPECT_EQ(pool.size(), 2u);
}
TEST(Values, CopyHasIndependentInterning) {
    ValuePool p; p.intern("%x"); auto copy = p;
    copy.intern("%y"); EXPECT_EQ(p.size(), 1u); EXPECT_EQ(copy.size(), 2u);
}
TEST(IRHelpers, TerminatorClassification) {
    for (const char* op : {"br", "ret", "switch", "indirectbr", "unreachable", "resume", "cleanupret", "catchret", "catchswitch", "callbr"}) {
        Instr in; in.opcode = op; EXPECT_TRUE(in.isTerminator()) << op;
    }
    for (const char* op : {"add", "phi", "call", "load", "select", ""}) {
        Instr in; in.opcode = op; EXPECT_FALSE(in.isTerminator()) << op;
    }
}
TEST(IRHelpers, BlockQueriesAndSortedVregs) {
    auto fn = fixtures::diamond();
    EXPECT_EQ(fn.instrCount(), 7u);
    EXPECT_EQ(fn.blocks[3].phis().size(), 1u);
    EXPECT_EQ(fn.blocks[3].terminator(), &fn.blocks[3].instrs.back());
    EXPECT_EQ(fn.vregs(), (std::vector<ValueId>{0, 1, 2, 3, 4}));
    fn.params.push_back(0); EXPECT_EQ(fn.vregs().size(), 5u);
    fn.blocks[0].instrs.clear(); EXPECT_EQ(fn.blocks[0].terminator(), nullptr);
}
TEST(IRHelpers, EmptyFunctionHelpers) {
    Function fn; EXPECT_EQ(fn.instrCount(), 0u); EXPECT_TRUE(fn.vregs().empty());
}
TEST(Bitsets, EmptyAndWordBoundaries) {
    BitSet empty; EXPECT_TRUE(empty.empty()); EXPECT_EQ(empty.count(), 0u);
    BitSet b(130);
    for (auto i : {0u, 63u, 64u, 127u, 129u}) b.set(i);
    EXPECT_EQ(b.count(), 5u);
    EXPECT_EQ(b.toVector(), (std::vector<ValueId>{0, 63, 64, 127, 129}));
    b.reset(64); EXPECT_FALSE(b.test(64)); EXPECT_TRUE(b.test(129));
    b.set(129); EXPECT_EQ(b.count(), 4u);
    std::vector<ValueId> visited; b.forEach([&](ValueId v) { visited.push_back(v); });
    EXPECT_EQ(visited, b.toVector());
}
TEST(Bitsets, SetAlgebraAndCopyIsolation) {
    auto a = fixtures::bits(65, {0, 64}), b = fixtures::bits(65, {1, 64});
    auto u = a; u |= b; EXPECT_EQ(u.toVector(), (std::vector<ValueId>{0, 1, 64}));
    auto i = a; i &= b; EXPECT_EQ(i.toVector(), (std::vector<ValueId>{64}));
    auto d = a; d -= b; EXPECT_EQ(d.toVector(), (std::vector<ValueId>{0}));
    EXPECT_TRUE(i.isSubsetOf(a)); EXPECT_FALSE(a.isSubsetOf(b));
    EXPECT_EQ(a.count(), 2u); EXPECT_FALSE(a == b); EXPECT_TRUE(a == a);
    a -= a; EXPECT_TRUE(a.empty()); b &= b; EXPECT_EQ(b.count(), 2u);
}
TEST(Bitsets, BoundsAndWidthChecks) {
    BitSet a(1), b(2), z;
    EXPECT_THROW(a.set(1), std::out_of_range);
    EXPECT_THROW(a.reset(1), std::out_of_range);
    EXPECT_THROW(a.test(1), std::out_of_range);
    EXPECT_THROW(z.set(0), std::out_of_range);
    EXPECT_THROW(a |= b, std::invalid_argument);
    EXPECT_THROW(a &= b, std::invalid_argument);
    EXPECT_THROW(a -= b, std::invalid_argument);
    EXPECT_THROW(a.isSubsetOf(b), std::invalid_argument);
    EXPECT_FALSE(a == b);
}
TEST(Graphs, SparseNodesAndSymmetricIdempotentEdges) {
    Graph g(8); g.addNode(7); g.addEdge(0, 4); g.addEdge(4, 0); g.addEdge(2, 2);
    EXPECT_EQ(g.nodes(), (std::vector<ValueId>{0, 4, 7}));
    EXPECT_EQ(g.numNodes(), 3u); EXPECT_EQ(g.numEdges(), 1u);
    EXPECT_TRUE(g.hasEdge(0, 4)); EXPECT_TRUE(g.hasEdge(4, 0));
    EXPECT_EQ(g.degree(7), 0u); EXPECT_FALSE(g.contains(2));
    EXPECT_EQ(g.neighbours(0).toVector(), (std::vector<ValueId>{4}));
    auto copy = g; copy.addEdge(4, 7); EXPECT_EQ(g.numEdges(), 1u);
}
TEST(Graphs, BoundsAndEmptyGraph) {
    Graph g(2), empty;
    EXPECT_TRUE(empty.nodes().empty()); EXPECT_EQ(empty.numEdges(), 0u);
    EXPECT_THROW(g.addNode(2), std::out_of_range);
    EXPECT_THROW(g.addEdge(0, kNoValue), std::out_of_range);
    EXPECT_THROW(g.hasEdge(2, 0), std::out_of_range);
    EXPECT_THROW(g.degree(2), std::out_of_range);
    EXPECT_THROW(g.neighbours(2), std::out_of_range);
    EXPECT_THROW(g.contains(kNoValue), std::out_of_range);
}
TEST(Graphs, NameOrderingAndBorrowedPool) {
    ValuePool pool; pool.intern("%z"); pool.intern("%a");
    Graph g(2); EXPECT_TRUE(g.nameLess(0, 1)); g.setPool(&pool);
    EXPECT_EQ(g.pool(), &pool); EXPECT_TRUE(g.nameLess(1, 0));
}
TEST(AllocationHelpers, CountsDistinctColoursExcludingSentinels) {
    AllocResult r; r.assignment = {0, 0, 3, kSpill, kUnassigned};
    EXPECT_EQ(r.coloursUsed(), 2); EXPECT_EQ(r.colourOf(4), kUnassigned);
    EXPECT_EQ(r.colourOf(100), kUnassigned); r.assignment.clear(); EXPECT_EQ(r.coloursUsed(), 0);
}
