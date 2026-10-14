#include "ssarv/phielim.hpp"

#include "ssarv/analysis.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace ssarv {

namespace {

struct Copy {
    ValueId dst = kNoValue;
    ValueId src = kNoValue;
    std::string constant;
    std::string type;

    bool isRegisterCopy() const { return src != kNoValue; }
};

std::string freshValueName(Function& fn, std::string_view stem,
                           std::size_t& next) {
    for (;;) {
        const auto name = "%" + std::string(stem) + "." + std::to_string(next++);
        if (!fn.values.lookup(name).has_value()) return name;
    }
}

std::string freshBlockLabel(Function& fn, std::size_t& next) {
    for (;;) {
        const auto label = "%phi.edge." + std::to_string(next++);
        if (!fn.labelToBlock.contains(label)) return label;
    }
}

/// Retarget the source-side representation of one CFG edge.  The resolved IDs
/// are the source of truth, but retaining the corresponding diagnostic labels
/// makes transformed IR easier to inspect and parse-independent clients safer.
void retargetSuccessor(Block& block, BlockId oldTarget, BlockId newTarget,
                       const std::string& oldLabel,
                       const std::string& newLabel) {
    bool found = false;
    for (auto& succ : block.succs) {
        if (succ == oldTarget) {
            succ = newTarget;
            found = true;
        }
    }
    if (!found) throw std::logic_error("critical edge is absent from source block");

    auto* term = block.instrs.empty() ? nullptr : &block.instrs.back();
    if (term == nullptr || !term->isTerminator()) {
        throw std::logic_error("critical-edge source has no terminator");
    }
    for (auto& succ : term->succs)
        if (succ == oldTarget) succ = newTarget;
    for (auto& label : term->succLabels)
        if (label == oldLabel) label = newLabel;
    for (auto& arm : term->cases) {
        if (arm.target == oldTarget) arm.target = newTarget;
        if (arm.targetLabel == oldLabel) arm.targetLabel = newLabel;
    }
}

void replacePredecessor(Block& block, BlockId oldPred, BlockId newPred,
                        const std::string& newLabel) {
    bool found = false;
    for (auto& pred : block.preds) {
        if (pred == oldPred) {
            pred = newPred;
            found = true;
        }
    }
    if (!found) throw std::logic_error("critical edge is absent from target block");

    for (auto& in : block.instrs) {
        if (!in.isPhi()) continue;
        for (auto& arg : in.phiArgs) {
            if (arg.pred == oldPred) {
                arg.pred = newPred;
                arg.predLabel = newLabel;
            }
        }
    }
}

Instr registerMove(ValueId dst, ValueId src, const std::string& type) {
    Instr move;
    move.opcode = "mov";
    move.type = type;
    move.dst = dst;
    move.uses = {src};
    return move;
}

Instr literalMove(ValueId dst, const std::string& constant,
                  const std::string& type) {
    Instr move;
    move.opcode = "mov";
    move.type = type;
    move.dst = dst;
    move.operands = {{kNoValue, constant}};
    return move;
}

void insertBeforeTerminator(Block& block, std::vector<Instr> moves) {
    if (moves.empty()) return;
    if (block.instrs.empty() || !block.instrs.back().isTerminator()) {
        throw std::logic_error("phi-copy edge block has no terminator");
    }
    block.instrs.insert(block.instrs.end() - 1,
                        std::make_move_iterator(moves.begin()),
                        std::make_move_iterator(moves.end()));
}

}  // namespace

std::vector<std::pair<ValueId, ValueId>> sequentialise(
    std::vector<std::pair<ValueId, ValueId>> pairs,
    const std::function<ValueId()>& makeTemp) {
    // A destination can safely be written when no remaining copy needs its
    // old value.  If every destination is still a source, the remaining
    // component contains a cycle; save one source before continuing.
    pairs.erase(std::remove_if(pairs.begin(), pairs.end(),
                               [](const auto& copy) { return copy.first == copy.second; }),
                pairs.end());

    std::vector<std::pair<ValueId, ValueId>> result;
    result.reserve(pairs.size() + 1);
    while (!pairs.empty()) {
        std::unordered_set<ValueId> sources;
        sources.reserve(pairs.size());
        for (const auto& [dst, src] : pairs) {
            (void)dst;
            sources.insert(src);
        }

        const auto safe = std::find_if(pairs.begin(), pairs.end(),
                                       [&](const auto& copy) {
                                           return !sources.contains(copy.first);
                                       });
        if (safe != pairs.end()) {
            result.push_back(*safe);
            pairs.erase(safe);
            continue;
        }

        // Break a cycle d <- s by preserving old s, then make this copy read
        // the temporary.  The replacement creates a safe destination on a
        // later iteration and works for cycles of any length.
        auto& cycleCopy = pairs.front();
        const ValueId temporary = makeTemp();
        if (temporary == kNoValue) {
            throw std::logic_error("parallel-copy temporary is invalid");
        }
        result.emplace_back(temporary, cycleCopy.second);
        cycleCopy.second = temporary;
    }
    return result;
}

int splitCriticalEdges(Function& fn) {
    // Snapshot original edges.  Each inserted edge block has exactly one
    // predecessor and successor, so inspecting it would never be useful, but
    // the snapshot also prevents a mutation-sensitive iteration from splitting
    // the same logical edge more than once.
    std::vector<std::pair<BlockId, BlockId>> critical;
    const auto originalBlockCount = fn.blocks.size();
    for (BlockId pred = 0; pred < originalBlockCount; ++pred) {
        const auto& source = fn.blocks[pred];
        if (source.succs.size() < 2) continue;
        for (BlockId succ : source.succs) {
            if (succ >= originalBlockCount) {
                throw std::logic_error("CFG successor is out of range");
            }
            if (fn.blocks[succ].preds.size() > 1) critical.emplace_back(pred, succ);
        }
    }

    std::size_t labelNumber = 0;
    int split = 0;
    for (const auto& [pred, succ] : critical) {
        // Earlier transformations only retarget their own edge.  A malformed
        // CFG with duplicate successor IDs cannot faithfully encode distinct
        // LLVM phi predecessors, so reject it rather than silently changing
        // multiple control-flow paths.
        const auto occurrences = static_cast<int>(
            std::count(fn.blocks[pred].succs.begin(), fn.blocks[pred].succs.end(), succ));
        if (occurrences != 1) {
            throw std::logic_error("critical edge is not uniquely represented");
        }

        const std::string oldLabel = fn.blocks[succ].label;
        const std::string label = freshBlockLabel(fn, labelNumber);
        const BlockId edge = static_cast<BlockId>(fn.blocks.size());

        // References are reacquired after push_back because it may reallocate
        // the block vector.
        retargetSuccessor(fn.blocks[pred], succ, edge, oldLabel, label);
        replacePredecessor(fn.blocks[succ], pred, edge, label);

        Block bridge;
        bridge.id = edge;
        bridge.label = label;
        bridge.preds = {pred};
        bridge.succs = {succ};
        Instr branch;
        branch.opcode = "br";
        branch.succs = {succ};
        branch.succLabels = {oldLabel};
        bridge.instrs.push_back(std::move(branch));
        fn.labelToBlock.emplace(label, edge);
        fn.blocks.push_back(std::move(bridge));
        ++split;
    }
    return split;
}

PhiElimStats eliminatePhis(Function& fn) {
    PhiElimStats stats;
    stats.criticalEdgesSplit = splitCriticalEdges(fn);

    // The Group 1 reachability contract gives the production pipeline a
    // deterministic definition of which executable phi nodes are transformed;
    // the isolated Group 3 test target supplies its declared fixture provider.
    const auto reachable = reachableBlocks(fn);
    std::size_t temporaryNumber = 0;

    for (BlockId target : reachable) {
        if (target >= fn.blocks.size()) {
            throw std::logic_error("reachability returned an out-of-range block");
        }

        const auto phis = fn.blocks[target].phis();
        if (phis.empty()) continue;

        std::unordered_map<BlockId, std::vector<Copy>> copiesByEdge;
        std::unordered_map<ValueId, std::string> typeByValue;
        for (const Instr* phi : phis) {
            if (phi->dst == kNoValue) throw std::logic_error("phi has no destination");
            typeByValue.emplace(phi->dst, phi->type);
            ++stats.phis;
            for (const auto& arg : phi->phiArgs) {
                if (arg.pred == kNoBlock || arg.pred >= fn.blocks.size()) {
                    throw std::logic_error("phi has an invalid predecessor");
                }
                Copy copy;
                copy.dst = phi->dst;
                copy.src = arg.value;
                copy.constant = arg.constant;
                copy.type = phi->type;
                copiesByEdge[arg.pred].push_back(std::move(copy));
            }
        }

        for (auto& [pred, copies] : copiesByEdge) {
            if (pred >= fn.blocks.size()) {
                throw std::logic_error("phi predecessor is out of range");
            }

            std::vector<std::pair<ValueId, ValueId>> registerPairs;
            std::vector<Copy> literals;
            for (const auto& copy : copies) {
                if (copy.isRegisterCopy()) {
                    if (copy.dst != copy.src) registerPairs.emplace_back(copy.dst, copy.src);
                } else {
                    literals.push_back(copy);
                }
            }

            const auto sequence = sequentialise(
                std::move(registerPairs), [&] {
                    const auto name = freshValueName(fn, "phi.tmp", temporaryNumber);
                    const auto value = fn.values.intern(name);
                    ++stats.cyclesBroken;
                    return value;
                });

            std::vector<Instr> moves;
            moves.reserve(sequence.size() + literals.size());
            for (const auto& [dst, src] : sequence) {
                auto type = typeByValue.find(dst);
                std::string moveType;
                if (type == typeByValue.end()) {
                    // A temporary is first defined by a saved cycle source.
                    // All members of an LLVM phi-copy cycle have one integer
                    // type, so the source's type identifies the temporary.
                    type = typeByValue.find(src);
                    if (type == typeByValue.end()) {
                        throw std::logic_error("cannot determine parallel-copy type");
                    }
                    moveType = type->second;
                    typeByValue.emplace(dst, moveType);
                } else {
                    moveType = type->second;
                }
                moves.push_back(registerMove(dst, src, moveType));
            }

            // Literals write destinations but are never sources.  Placing them
            // last preserves an old register value needed by another copy on
            // this edge (for example, {a <- 7, b <- a}).
            for (const auto& copy : literals) {
                moves.push_back(literalMove(copy.dst, copy.constant, copy.type));
            }
            stats.copies += static_cast<int>(moves.size());
            insertBeforeTerminator(fn.blocks[pred], std::move(moves));
        }

        auto& instructions = fn.blocks[target].instrs;
        instructions.erase(std::remove_if(instructions.begin(), instructions.end(),
                                          [](const Instr& in) { return in.isPhi(); }),
                           instructions.end());
    }
    return stats;
}

} // namespace ssarv
