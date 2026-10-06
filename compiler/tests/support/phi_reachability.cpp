// Test-only declaration provider. No CFG traversal or analysis algorithm.
#include "support/phi_reachability.hpp"
#include "ssarv/analysis.hpp"
#include <numeric>
#include <stdexcept>
#include <unordered_set>
namespace {
std::unordered_set<const ssarv::Function*> registered;
}
namespace fixtures {
AllReachableFixture::AllReachableFixture(const ssarv::Function& fn) : fn_(&fn) {
    if (!registered.insert(fn_).second) throw std::logic_error("Duplicate phi fixture");
}
AllReachableFixture::~AllReachableFixture() { registered.erase(fn_); }
}
namespace ssarv {
std::vector<BlockId> reachableBlocks(const Function& fn) {
    if (!registered.contains(&fn)) throw std::logic_error("No declared phi reachability fixture");
    std::vector<BlockId> ids(fn.blocks.size());
    std::iota(ids.begin(), ids.end(), BlockId{0});
    return ids;
}
}
