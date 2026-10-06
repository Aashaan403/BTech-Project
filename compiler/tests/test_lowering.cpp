#include <gtest/gtest.h>
#include "support/recording_context.hpp"
using namespace ssarv;
TEST(Lowering, BinaryCommitsThroughContextAndUsesReservedScratch) {
    fixtures::RecordingContext ctx; ctx.registers = {{0,"a0"},{1,"a1"},{2,"a0"}};
    Instr in; in.opcode = "add"; in.type = "i32"; in.dst = 2; in.uses = {0,1};
    in.operands = {{0,"%x"},{1,"%y"}}; lowerBinary(in, ctx);
    ASSERT_EQ(ctx.commits.size(), 1u); EXPECT_EQ(ctx.commits[0].first, 2u);
    EXPECT_EQ(ctx.commits[0].second, "a0"); EXPECT_FALSE(ctx.lines.empty());
    for (const auto& scratch : ctx.requestedScratch) EXPECT_TRUE(scratch == "t5" || scratch == "t6");
}
TEST(Lowering, RegisterAndLiteralMovesCommitSpilledDestination) {
    for (bool literal : {false,true}) {
        fixtures::RecordingContext ctx; ctx.registers = {{0,"a0"}};
        Instr in; in.opcode = "mov"; in.type = "i32"; in.dst = 1;
        if (literal) in.operands = {{kNoValue,"17"}}; else in.uses = {0};
        lowerMove(in, ctx); ASSERT_EQ(ctx.commits.size(), 1u); EXPECT_EQ(ctx.commits[0].first, 1u);
    }
}
TEST(Lowering, CompareCastAndSelectUseContextResults) {
    for (const char* opcode : {"icmp", "trunc", "select"}) {
        fixtures::RecordingContext ctx; ctx.registers = {{0,"a0"},{1,"a1"},{2,"a2"}};
        Instr in; in.opcode = opcode; in.type = "i32"; in.dst = 2;
        in.operands = {{0,"%x"},{1,"%y"}};
        if (in.opcode == "icmp") { in.subop = "ult"; lowerCompare(in,ctx); }
        else if (in.opcode == "trunc") { in.operands.resize(1); in.text = "trunc i32 %x to i1"; lowerCast(in,ctx); }
        else { in.operands = {{0,"%c"},{1,"%x"},{kNoValue,"3"}}; lowerSelect(in,ctx); }
        ASSERT_EQ(ctx.commits.size(), 1u); EXPECT_EQ(ctx.commits[0].first, 2u);
        EXPECT_FALSE(ctx.lines.empty());
    }
}
TEST(Lowering, BranchAndReturnUseContextLabels) {
    fixtures::RecordingContext ctx; ctx.registers = {{0,"a0"}};
    Instr br; br.opcode = "br"; br.succs = {3}; lowerTerminator(br,ctx);
    EXPECT_NE(ctx.text().find(".Lblock3"), std::string::npos);
    Instr ret; ret.opcode = "ret"; ret.type = "i32"; ret.operands = {{0,"%x"}};
    lowerTerminator(ret,ctx); EXPECT_NE(ctx.text().find(".Lepilogue"), std::string::npos);
    EXPECT_TRUE(ctx.commits.empty());
}
