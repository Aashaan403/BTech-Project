#include "ssarv/allocators.hpp"
#include <set>
namespace ssarv {
int AllocResult::coloursUsed() const {
    std::set<int> colours;
    for (int colour : assignment) if (colour >= 0) colours.insert(colour);
    return static_cast<int>(colours.size());
}
} // namespace ssarv
