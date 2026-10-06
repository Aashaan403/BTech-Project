// Test-only data provider. Never link into production compiler targets.
#include "support/analysis_provider.hpp"
#include "support/fixtures.hpp"
#include <stdexcept>
#include <unordered_map>

namespace {
std::unordered_map<const ssarv::Function*, fixtures::AnalysisData> records;
const fixtures::AnalysisData& lookup(const ssarv::Function& fn) {
    auto it = records.find(&fn);
    if (it == records.end()) throw std::logic_error("No registered analysis fixture");
    return it->second;
}
}
namespace fixtures {
AnalysisFixture::AnalysisFixture(const ssarv::Function& fn, AnalysisData data) : fn_(&fn) {
    if (!records.emplace(fn_, std::move(data)).second)
        throw std::logic_error("Duplicate analysis fixture");
}
AnalysisFixture::~AnalysisFixture() { records.erase(fn_); }
AnalysisData straightAnalysis() {
    return {{0}, {0}, {0}, {{bits(2, {0})}, {bits(2, {})}}};
}
AnalysisData diamondAnalysis() {
    return {{0, 1, 2, 3}, {0, 2, 1, 3}, {0, 0, 0, 0},
        {{bits(5, {0, 1}), bits(5, {1}), bits(5, {1}), bits(5, {4})},
         {bits(5, {1}), bits(5, {2}), bits(5, {3}), bits(5, {})}}};
}
AnalysisData loopAnalysis() {
    return {{0, 1, 2, 3}, {0, 1, 3, 2}, {0, 1, 1, 1},
        {{bits(1, {0}), bits(1, {0}), bits(1, {0}), bits(1, {0})},
         {bits(1, {0}), bits(1, {0}), bits(1, {0}), bits(1, {0})}}};
}
}
namespace ssarv {
std::vector<BlockId> reachableBlocks(const Function& fn) { return lookup(fn).reachable; }
std::vector<BlockId> reversePostorder(const Function& fn) { return lookup(fn).rpo; }
std::vector<int> loopDepths(const Function& fn) { return lookup(fn).depths; }
Liveness computeLiveness(const Function& fn) { return lookup(fn).liveness; }
}
