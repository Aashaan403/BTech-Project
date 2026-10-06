#include <gtest/gtest.h>
#include "ssarv/verify.hpp"
#include "support/fixtures.hpp"
using namespace ssarv;
TEST(Lead, ParserResolvesSimpleFunction) {
    auto module = parseLLText("define i32 @f(i32 %x) {\nentry:\n ret i32 %x\n}\n");
    ASSERT_TRUE(module.has_value()) << module.error(); ASSERT_EQ(module->functions.size(), 1u);
    EXPECT_EQ(module->functions[0].name, "f"); EXPECT_EQ(module->functions[0].params.size(), 1u);
}
TEST(Lead, ChordalityDistinguishesTriangleAndChordlessCycle) {
    Graph triangle(3); triangle.addEdge(0,1); triangle.addEdge(1,2); triangle.addEdge(2,0);
    auto chord = isChordal(triangle); EXPECT_TRUE(chord.chordal);
    EXPECT_EQ(cliqueNumberChordal(triangle,chord.eliminationOrder), 3u);
    Graph cycle(4); cycle.addEdge(0,1); cycle.addEdge(1,2); cycle.addEdge(2,3); cycle.addEdge(3,0);
    EXPECT_FALSE(isChordal(cycle).chordal);
}
TEST(Lead, VerifierFindsConflictAndSpillMismatch) {
    auto fn = fixtures::diamond(); auto g = fixtures::diamondGraph(fn);
    AllocResult r; r.k = 2; r.assignment = {0,0,0,0,0};
    EXPECT_FALSE(verifyAllocation(fn,g,r).empty());
    r.assignment = {0,1,0,0,kSpill}; EXPECT_FALSE(verifyAllocation(fn,g,r).empty());
    r.spilled = {4}; EXPECT_TRUE(verifyAllocation(fn,g,r).empty());
}
