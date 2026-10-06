// Analysis layer: dominators, natural loops, liveness, interference graph
// and the Maximum Cardinality Search chordality test.
//
// Virtual registers are dense ids, so adjacency is stored as bitsets.
#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "ssarv/ir.hpp"

namespace ssarv {

/// Fixed-width bitset over dense value ids.
class BitSet {
public:
    BitSet() = default;
    explicit BitSet(std::size_t n);

    void set(std::size_t i);
    void reset(std::size_t i);
    bool test(std::size_t i) const;

    std::size_t size() const { return n_; }
    std::size_t count() const;
    bool empty() const;

    BitSet& operator|=(const BitSet& o);
    BitSet& operator-=(const BitSet& o);
    BitSet& operator&=(const BitSet& o);
    bool operator==(const BitSet& o) const;

    /// True when every bit of *this is also set in `o`.
    bool isSubsetOf(const BitSet& o) const;

    template <class F>
    void forEach(F&& visit) const {
        for (std::size_t i = 0; i < w_.size(); ++i) {
            auto word = w_[i];
            while (word != 0) {
                visit(static_cast<ValueId>(i * 64 + std::countr_zero(word)));
                word &= word - 1;
            }
        }
    }

    std::vector<ValueId> toVector() const;

private:
    std::size_t n_ = 0;
    std::vector<std::uint64_t> w_;
};

/// Undirected graph over dense value ids, bitset adjacency.
class Graph {
public:
    explicit Graph(std::size_t n = 0);

    void addNode(ValueId v);
    void addEdge(ValueId a, ValueId b);
    bool hasEdge(ValueId a, ValueId b) const;
    std::size_t degree(ValueId v) const;
    const BitSet& neighbours(ValueId v) const;
    bool contains(ValueId v) const;

    /// Optional name table for deterministic tie-breaking.
    void setPool(const ValuePool* p) { pool_ = p; }
    const ValuePool* pool() const { return pool_; }
    /// Tie-break comparison: by name when a pool is attached, else by id.
    bool nameLess(ValueId a, ValueId b) const {
        return pool_ ? pool_->name(a) < pool_->name(b) : a < b;
    }

    std::size_t capacity() const { return n_; }
    std::vector<ValueId> nodes() const;
    std::size_t numNodes() const;
    std::size_t numEdges() const;

private:
    std::size_t n_ = 0;
    std::vector<BitSet> adj_;
    std::vector<bool> present_;
    const ValuePool* pool_ = nullptr;
};

// ----------------------------------------------------------------- CFG
std::vector<BlockId> reachableBlocks(const Function& fn);
std::vector<BlockId> reversePostorder(const Function& fn);

// ---------------------------------------------------------- dominators
/// Immediate dominator of each reachable block (entry maps to itself).
std::vector<BlockId> dominators(const Function& fn);
bool dominates(const std::vector<BlockId>& idom, BlockId a, BlockId b);

/// Loop nesting depth of each block (0 == not in a loop).
std::vector<int> loopDepths(const Function& fn);

// ------------------------------------------------------------ liveness
struct Liveness {
    std::vector<BitSet> liveIn;
    std::vector<BitSet> liveOut;
};
Liveness computeLiveness(const Function& fn);

// ------------------------------------------------- interference graph
struct Interference {
    Graph graph;
    std::size_t maxlive = 0;
};
Interference interferenceGraph(const Function& fn);

// -------------------------------------------- maximum cardinality search
/// MCS visit order; its reverse is a perfect elimination ordering candidate.
std::vector<ValueId> mcsOrder(const Graph& g);
bool isPerfectEliminationOrder(const Graph& g, const std::vector<ValueId>& order);

struct Chordality {
    bool chordal = false;
    std::vector<ValueId> eliminationOrder;
};
Chordality isChordal(const Graph& g);

/// omega -- exact only when the graph is chordal.
std::size_t cliqueNumberChordal(const Graph& g, const std::vector<ValueId>& elim);

}  // namespace ssarv
