// LLVM IR (.ll) subset parser and in-memory SSA representation.
//
// Virtual registers are interned to dense integer ids so that downstream
// analysis can use bitsets rather than hash sets.
#pragma once

#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ssarv {

using ValueId = std::uint32_t;
using BlockId = std::uint32_t;

inline constexpr ValueId kNoValue = static_cast<ValueId>(-1);
inline constexpr BlockId kNoBlock = static_cast<BlockId>(-1);

/// Interns %-prefixed virtual register names as dense ids.
class ValuePool {
public:
    ValueId intern(std::string_view name);
    std::optional<ValueId> lookup(std::string_view name) const;
    const std::string& name(ValueId id) const { return names_[id]; }
    std::size_t size() const { return names_.size(); }

private:
    std::vector<std::string> names_;
    std::unordered_map<std::string, ValueId> index_;
};

struct PhiArg {
    ValueId value = kNoValue;   ///< kNoValue when the operand is a constant
    std::string constant;       ///< text of that constant
    BlockId pred = kNoBlock;
    std::string predLabel;      ///< filled during parsing, resolved afterwards
};

/// One operand of an instruction: either a virtual register or a literal.
struct Operand {
    ValueId value = kNoValue;   ///< kNoValue when this is a literal
    std::string text;           ///< literal text, or the register name
    bool isReg() const { return value != kNoValue; }
};

/// One arm of a `switch`: a constant value and the block it jumps to.
struct SwitchCase {
    std::string value;
    BlockId target = kNoBlock;
    std::string targetLabel;
};

struct Instr {
    std::string opcode;
    std::string subop;          ///< icmp predicate, etc.
    std::string type;           ///< operand type, e.g. "i32"
    std::vector<Operand> operands;
    ValueId dst = kNoValue;
    std::vector<ValueId> uses;
    std::vector<PhiArg> phiArgs;
    std::vector<SwitchCase> cases;   ///< switch arms (default is succs[0])
    std::vector<BlockId> succs;
    std::vector<std::string> succLabels;   ///< resolved into succs afterwards
    std::string text;

    bool isPhi() const { return opcode == "phi"; }
    bool isTerminator() const;
};

struct Block {
    BlockId id = kNoBlock;
    std::string label;
    std::vector<Instr> instrs;
    std::vector<BlockId> preds;
    std::vector<BlockId> succs;

    const Instr* terminator() const;
    std::vector<const Instr*> phis() const;
};

struct Function {
    std::string name;
    std::vector<ValueId> params;
    std::vector<Block> blocks;
    ValuePool values;
    std::unordered_map<std::string, BlockId> labelToBlock;

    BlockId entry() const { return blocks.empty() ? kNoBlock : blocks.front().id; }
    const Block& block(BlockId id) const { return blocks[id]; }
    Block& block(BlockId id) { return blocks[id]; }
    std::size_t instrCount() const;
    /// Every value needing a register: parameters plus instruction results.
    std::vector<ValueId> vregs() const;
    std::size_t numValues() const { return values.size(); }
};

/// A module-level variable. Only zero-initialised aggregates are modelled,
/// which is all `clang -O1` produces for these benchmarks.
struct Global {
    std::string name;
    std::size_t size = 0;
    std::size_t align = 1;
    bool zeroInit = true;
    /// Scalar integer initialiser when !zeroInit and it is representable.
    bool scalarInit = false;
    long long initValue = 0;
    std::string initText;     ///< raw initialiser, for diagnostics
};

/// Size in bytes of an LLVM type spelling such as "[12 x [12 x i32]]".
std::size_t typeSizeBytes(std::string_view type);

struct Module {
    std::string name;
    std::string targetTriple;
    std::vector<Function> functions;
    std::vector<Global> globals;
};

std::expected<Module, std::string> parseLLText(std::string_view text,
                                               std::string name = "<memory>");
std::expected<Module, std::string> parseLL(const std::string& path);

}  // namespace ssarv
