#include <gtest/gtest.h>
#include <type_traits>
#include "ssarv/allocators.hpp"
#include "ssarv/backend.hpp"
#include "ssarv/coalesce.hpp"
#include "ssarv/driver.hpp"
#include "ssarv/lowering.hpp"
#include "ssarv/phielim.hpp"
#include "ssarv/verify.hpp"

TEST(Scaffold, DenseIdSentinelsAndDefaults) {
    ssarv::Function fn;
    EXPECT_EQ(fn.entry(), ssarv::kNoBlock);
    EXPECT_EQ(fn.numValues(), 0u);
    ssarv::Instr in;
    EXPECT_EQ(in.dst, ssarv::kNoValue);
    EXPECT_FALSE(in.isPhi());
    EXPECT_FALSE(ssarv::Operand{}.isReg());
}
TEST(Scaffold, AllocationAndBackendInterfaces) {
    ssarv::AllocResult r;
    EXPECT_EQ(r.colourOf(0), ssarv::kUnassigned);
    EXPECT_FALSE(r.exact);
    EXPECT_EQ(ssarv::kNumAllocatable, 25);
    EXPECT_STREQ(ssarv::kScratch0, "t5");
    EXPECT_STREQ(ssarv::kScratch1, "t6");
    static_assert(std::is_abstract_v<ssarv::LoweringContext>);
    static_assert(std::is_same_v<decltype(&ssarv::runCompiler), int(*)(int, char**)>);
}
