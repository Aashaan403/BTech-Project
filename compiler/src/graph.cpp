#include "ssarv/analysis.hpp"
#include <algorithm>
#include <stdexcept>

namespace ssarv {
Graph::Graph(std::size_t n) : n_(n) {
    if (n > kNoValue) throw std::length_error("Graph exceeds dense ValueId space");
    adj_.assign(n, BitSet(n));
    present_.assign(n, false);
}
void Graph::addNode(ValueId v) {
    if (v >= n_) throw std::out_of_range("Graph node ID");
    present_[v] = true;
}
void Graph::addEdge(ValueId a, ValueId b) {
    if (a >= n_ || b >= n_) throw std::out_of_range("Graph edge endpoint");
    if (a == b) return;
    present_[a] = present_[b] = true;
    adj_[a].set(b); adj_[b].set(a);
}
bool Graph::hasEdge(ValueId a, ValueId b) const {
    if (a >= n_ || b >= n_) throw std::out_of_range("Graph edge endpoint");
    return adj_[a].test(b);
}
std::size_t Graph::degree(ValueId v) const { return neighbours(v).count(); }
const BitSet& Graph::neighbours(ValueId v) const {
    if (v >= n_) throw std::out_of_range("Graph node ID");
    return adj_[v];
}
bool Graph::contains(ValueId v) const {
    if (v >= n_) throw std::out_of_range("Graph node ID");
    return present_[v];
}
std::vector<ValueId> Graph::nodes() const {
    std::vector<ValueId> result;
    for (std::size_t i = 0; i < n_; ++i)
        if (present_[i]) result.push_back(static_cast<ValueId>(i));
    return result;
}
std::size_t Graph::numNodes() const {
    return static_cast<std::size_t>(std::ranges::count(present_, true));
}
std::size_t Graph::numEdges() const {
    std::size_t sum = 0;
    for (const auto& neighbours : adj_) sum += neighbours.count();
    return sum / 2;
}
} // namespace ssarv
