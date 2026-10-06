#pragma once
#include "ssarv/analysis.hpp"

namespace fixtures {
struct AnalysisData {
    std::vector<ssarv::BlockId> reachable, rpo;
    std::vector<int> depths;
    ssarv::Liveness liveness;
};
// Test-only RAII registration. Does not compute any analysis.
class AnalysisFixture {
public:
    AnalysisFixture(const ssarv::Function& fn, AnalysisData data);
    ~AnalysisFixture();
    AnalysisFixture(const AnalysisFixture&) = delete;
    AnalysisFixture& operator=(const AnalysisFixture&) = delete;
private:
    const ssarv::Function* fn_;
};
AnalysisData straightAnalysis();
AnalysisData diamondAnalysis();
AnalysisData loopAnalysis();
} // namespace fixtures
