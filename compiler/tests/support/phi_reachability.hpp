#pragma once
#include "ssarv/ir.hpp"
namespace fixtures {
// Test declaration: every block in this fixture is reachable, including
// critical-edge blocks later added by transformation. No traversal is run.
class AllReachableFixture {
public:
    explicit AllReachableFixture(const ssarv::Function& fn);
    ~AllReachableFixture();
    AllReachableFixture(const AllReachableFixture&) = delete;
    AllReachableFixture& operator=(const AllReachableFixture&) = delete;
private:
    const ssarv::Function* fn_;
};
}
