#include "ssarv/analysis.hpp"
namespace ssarv {
BitSet::BitSet(std::size_t) { unimplemented("BitSet::BitSet"); }
void BitSet::set(std::size_t) { unimplemented("BitSet::set"); }
void BitSet::reset(std::size_t) { unimplemented("BitSet::reset"); }
bool BitSet::test(std::size_t) const { unimplemented("BitSet::test"); }
std::size_t BitSet::count() const { unimplemented("BitSet::count"); }
bool BitSet::empty() const { unimplemented("BitSet::empty"); }
BitSet& BitSet::operator|=(const BitSet&) { unimplemented("BitSet::union"); }
BitSet& BitSet::operator-=(const BitSet&) { unimplemented("BitSet::difference"); }
BitSet& BitSet::operator&=(const BitSet&) { unimplemented("BitSet::intersection"); }
bool BitSet::operator==(const BitSet&) const { unimplemented("BitSet::equality"); }
bool BitSet::isSubsetOf(const BitSet&) const { unimplemented("BitSet::isSubsetOf"); }
std::vector<ValueId> BitSet::toVector() const { unimplemented("BitSet::toVector"); }
} // namespace ssarv
