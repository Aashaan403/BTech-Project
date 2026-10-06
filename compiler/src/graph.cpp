#include "ssarv/analysis.hpp"
namespace ssarv {
Graph::Graph(std::size_t) { unimplemented("Graph::Graph"); }
void Graph::addNode(ValueId) { unimplemented("Graph::addNode"); }
void Graph::addEdge(ValueId, ValueId) { unimplemented("Graph::addEdge"); }
bool Graph::hasEdge(ValueId, ValueId) const { unimplemented("Graph::hasEdge"); }
std::size_t Graph::degree(ValueId) const { unimplemented("Graph::degree"); }
const BitSet& Graph::neighbours(ValueId) const { unimplemented("Graph::neighbours"); }
bool Graph::contains(ValueId) const { unimplemented("Graph::contains"); }
std::vector<ValueId> Graph::nodes() const { unimplemented("Graph::nodes"); }
std::size_t Graph::numNodes() const { unimplemented("Graph::numNodes"); }
std::size_t Graph::numEdges() const { unimplemented("Graph::numEdges"); }
} // namespace ssarv
