#include <gtest/gtest.h>
#include "ssarv/backend.hpp"
#include "ssarv/phielim.hpp"
#include "ssarv/verify.hpp"
using namespace ssarv;
TEST(Integration, ProductionPipelineProducesAssemblyForSimpleFunction) {
    auto module = parseLLText("define i32 @f(i32 %x) {\nentry:\n %y = add i32 %x, 1\n ret i32 %y\n}\n");
    ASSERT_TRUE(module.has_value()) << module.error(); ASSERT_EQ(module->functions.size(), 1u);
    auto& fn = module->functions[0]; eliminatePhis(fn);
    auto graph = interferenceGraph(fn).graph;
    auto allocation = allocChaitinBriggs(fn,graph,6);
    ASSERT_TRUE(verifyAllocation(fn,graph,allocation).empty());
    auto result = emitFunction(fn,allocation); ASSERT_TRUE(result.ok) << result.reason;
    EXPECT_FALSE(result.asmText.empty()); EXPECT_NE(result.asmText.find("f:"), std::string::npos);
}
