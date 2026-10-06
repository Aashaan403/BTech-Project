#include "ssarv/analysis.hpp"
#include <bit>
#include <stdexcept>

namespace ssarv {
BitSet::BitSet(std::size_t n) : n_(n) {
    if (n > kNoValue) throw std::length_error("Bitset exceeds dense ValueId space");
    w_.assign(n / 64 + (n % 64 != 0), 0);
}
void BitSet::set(std::size_t i) {
    if (i >= n_) throw std::out_of_range("Bitset index");
    w_[i / 64] |= std::uint64_t{1} << (i % 64);
}
void BitSet::reset(std::size_t i) {
    if (i >= n_) throw std::out_of_range("Bitset index");
    w_[i / 64] &= ~(std::uint64_t{1} << (i % 64));
}
bool BitSet::test(std::size_t i) const {
    if (i >= n_) throw std::out_of_range("Bitset index");
    return (w_[i / 64] & (std::uint64_t{1} << (i % 64))) != 0;
}
std::size_t BitSet::count() const {
    std::size_t count = 0;
    for (auto word : w_) count += std::popcount(word);
    return count;
}
bool BitSet::empty() const {
    for (auto word : w_) if (word != 0) return false;
    return true;
}
BitSet& BitSet::operator|=(const BitSet& other) {
    if (n_ != other.n_) throw std::invalid_argument("Bitset width mismatch");
    for (std::size_t i = 0; i < w_.size(); ++i) w_[i] |= other.w_[i];
    return *this;
}
BitSet& BitSet::operator-=(const BitSet& other) {
    if (n_ != other.n_) throw std::invalid_argument("Bitset width mismatch");
    for (std::size_t i = 0; i < w_.size(); ++i) w_[i] &= ~other.w_[i];
    return *this;
}
BitSet& BitSet::operator&=(const BitSet& other) {
    if (n_ != other.n_) throw std::invalid_argument("Bitset width mismatch");
    for (std::size_t i = 0; i < w_.size(); ++i) w_[i] &= other.w_[i];
    return *this;
}
bool BitSet::operator==(const BitSet& other) const { return n_ == other.n_ && w_ == other.w_; }
bool BitSet::isSubsetOf(const BitSet& other) const {
    if (n_ != other.n_) throw std::invalid_argument("Bitset width mismatch");
    for (std::size_t i = 0; i < w_.size(); ++i)
        if ((w_[i] & ~other.w_[i]) != 0) return false;
    return true;
}
std::vector<ValueId> BitSet::toVector() const {
    std::vector<ValueId> result;
    result.reserve(count());
    forEach([&](ValueId v) { result.push_back(v); });
    return result;
}
} // namespace ssarv
