// RISC-V (RV64IM) assembly emission for an allocated function.
#pragma once
#include <string>
#include <vector>
#include "ssarv/allocators.hpp"

namespace ssarv {

/// Allocatable integer registers, in colour order.  x0 is hardwired zero;
/// ra/sp/gp/tp are reserved by the ABI; t5 and t6 are held back as scratch
/// for reloading spilled values.
extern const char* const kRegNames[];
inline constexpr int kNumAllocatable = 25;
inline constexpr const char* kScratch0 = "t5";
inline constexpr const char* kScratch1 = "t6";

struct EmitResult {
    bool ok = false;
    std::string asmText;
    std::string reason;              ///< why it was rejected, when !ok
    std::vector<std::string> unsupported;
    int spillSlots = 0;
};

/// Is every opcode in this function one the emitter can lower?
bool isEmittable(const Function& fn, std::vector<std::string>* unsupportedOut = nullptr);

/// Emit assembly. The function must already be out of SSA (phis eliminated).
EmitResult emitFunction(const Function& fn, const AllocResult& alloc);

}  // namespace ssarv
