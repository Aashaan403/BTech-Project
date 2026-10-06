#include <gtest/gtest.h>
#include <algorithm>
#include <set>
#include <stdexcept>
#include "ssarv/allocators.hpp"
#include "support/fixtures.hpp"
#include "support/analysis_provider.hpp"
using namespace ssarv;
namespace {
// Assertions here are test invariants, independent of Group 4's production verifier.
void valid(const Function& fn, const Graph& g, const AllocResult& r, int k) {
    ASSERT_EQ(r.assignment.size(), fn.numValues()); EXPECT_EQ(r.k, k); EXPECT_FALSE(r.exact);
    std::set<ValueId> spills(r.spilled.begin(), r.spilled.end());
    EXPECT_EQ(spills.size(), r.spilled.size());
    for (auto v : r.spilled) { ASSERT_LT(v, fn.numValues()); EXPECT_TRUE(g.contains(v)); }
    for (auto v : g.nodes()) {
        auto c = r.colourOf(v);
        EXPECT_TRUE(c == kSpill || (c >= 0 && c < k));
        EXPECT_EQ(spills.contains(v), c == kSpill);
        if (c >= 0) g.neighbours(v).forEach([&](ValueId u) { EXPECT_NE(c, r.colourOf(u)); });
    }
}
}
TEST(Allocators, EmptyAndInvalidBudgets) {
    Function fn; Graph g; fixtures::AnalysisFixture fixture(fn, {});
    for (auto allocate : {allocNaive, allocLinearScan, allocChaitinBriggs}) {
        auto r = allocate(fn, g, 1); EXPECT_TRUE(r.assignment.empty()); EXPECT_TRUE(r.spilled.empty());
        EXPECT_THROW(allocate(fn, g, 0), std::invalid_argument);
        EXPECT_THROW(allocate(fn, g, 26), std::invalid_argument);
    }
}
TEST(Allocators, NaiveAndHeuristicsOnManuallySpecifiedDiamond) {
    auto fn = fixtures::diamond(); auto g = fixtures::diamondGraph(fn); g.setPool(&fn.values);
    fixtures::AnalysisFixture fixture(fn, fixtures::diamondAnalysis());
    for (int k : {1,2,6,25}) {
        auto naive = allocNaive(fn, g, k); valid(fn, g, naive, k);
        EXPECT_EQ(naive.name, "naive"); EXPECT_EQ(naive.spilled.size(), g.numNodes());
        auto linear = allocLinearScan(fn, g, k), cb = allocChaitinBriggs(fn, g, k);
        valid(fn, g, linear, k); valid(fn, g, cb, k);
        EXPECT_EQ(linear.name, "linear_scan"); EXPECT_EQ(cb.name, "chaitin_briggs");
        if (k == 25) { EXPECT_TRUE(linear.spilled.empty()); EXPECT_TRUE(cb.spilled.empty()); }
        EXPECT_EQ(cb.assignment, allocChaitinBriggs(fn, g, k).assignment);
    }
}
TEST(Allocators, UniqueUsesAndLoopWeightedCosts) {
    auto straight = fixtures::straight();
    fixtures::AnalysisFixture a(straight, fixtures::straightAnalysis());
    EXPECT_EQ(spillCosts(straight), (std::vector<double>{2,2}));
    auto loop = fixtures::twoLatchLoop();
    fixtures::AnalysisFixture b(loop, fixtures::loopAnalysis());
    EXPECT_EQ(spillCosts(loop), (std::vector<double>{11}));
}
TEST(FixtureProvider, UnknownFunctionsFailInsteadOfComputingAnalysis) {
    Function fn;
    EXPECT_THROW(computeLiveness(fn), std::logic_error);
    EXPECT_THROW(loopDepths(fn), std::logic_error);
}
