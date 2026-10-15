#include "ssarv/analysis.hpp"

#include <algorithm>
#include <cstddef>
#include <unordered_map>
#include <vector>

namespace ssarv {

std::vector<BlockId> reachableBlocks(const Function& fn) {
    if (fn.blocks.empty()) return {};

    BlockId entry = fn.entry();
    if (entry == kNoBlock || entry >= fn.blocks.size()) return {};

    std::vector<BlockId> reachable;
    reachable.reserve(fn.blocks.size());
    std::vector<bool> visited(fn.blocks.size(), false);

    struct Frame {
        BlockId b;
        std::size_t nextSucc;
    };

    std::vector<Frame> stack;
    stack.reserve(fn.blocks.size());

    visited[entry] = true;
    reachable.push_back(entry);
    stack.push_back({entry, 0});

    while (!stack.empty()) {
        auto& [b, nextSucc] = stack.back();
        const auto& succs = fn.blocks[b].succs;
        if (nextSucc < succs.size()) {
            BlockId s = succs[nextSucc++];
            if (s < fn.blocks.size() && !visited[s]) {
                visited[s] = true;
                reachable.push_back(s);
                stack.push_back({s, 0});
            }
        } else {
            stack.pop_back();
        }
    }

    return reachable;
}

std::vector<BlockId> reversePostorder(const Function& fn) {
    if (fn.blocks.empty()) return {};

    BlockId entry = fn.entry();
    if (entry == kNoBlock || entry >= fn.blocks.size()) return {};

    std::vector<bool> visited(fn.blocks.size(), false);
    std::vector<BlockId> postorder;
    postorder.reserve(fn.blocks.size());

    struct Frame {
        BlockId b;
        std::size_t nextSucc;
    };

    std::vector<Frame> stack;
    stack.reserve(fn.blocks.size());

    visited[entry] = true;
    stack.push_back({entry, 0});

    while (!stack.empty()) {
        auto& [b, nextSucc] = stack.back();
        const auto& succs = fn.blocks[b].succs;
        if (nextSucc < succs.size()) {
            BlockId s = succs[nextSucc++];
            if (s < fn.blocks.size() && !visited[s]) {
                visited[s] = true;
                stack.push_back({s, 0});
            }
        } else {
            postorder.push_back(b);
            stack.pop_back();
        }
    }

    std::reverse(postorder.begin(), postorder.end());
    return postorder;
}

std::vector<BlockId> dominators(const Function& fn) {
    if (fn.blocks.empty()) return {};

    const std::size_t numBlocks = fn.blocks.size();
    std::vector<BlockId> idom(numBlocks, kNoBlock);

    auto rpo = reversePostorder(fn);
    if (rpo.empty()) return idom;

    std::vector<std::size_t> rpo_idx(numBlocks, static_cast<std::size_t>(-1));
    for (std::size_t i = 0; i < rpo.size(); ++i) {
        rpo_idx[rpo[i]] = i;
    }

    BlockId entry = fn.entry();
    idom[entry] = entry;

    auto intersect = [&](BlockId b1, BlockId b2) -> BlockId {
        BlockId finger1 = b1;
        BlockId finger2 = b2;
        while (finger1 != finger2) {
            while (rpo_idx[finger1] > rpo_idx[finger2]) {
                finger1 = idom[finger1];
            }
            while (rpo_idx[finger2] > rpo_idx[finger1]) {
                finger2 = idom[finger2];
            }
        }
        return finger1;
    };

    bool changed = true;
    while (changed) {
        changed = false;
        for (BlockId b : rpo) {
            if (b == entry) continue;

            BlockId new_idom = kNoBlock;
            for (BlockId p : fn.blocks[b].preds) {
                if (p < numBlocks && idom[p] != kNoBlock) {
                    new_idom = p;
                    break;
                }
            }
            if (new_idom == kNoBlock) continue;

            for (BlockId p : fn.blocks[b].preds) {
                if (p < numBlocks && p != new_idom && idom[p] != kNoBlock) {
                    new_idom = intersect(p, new_idom);
                }
            }

            if (idom[b] != new_idom) {
                idom[b] = new_idom;
                changed = true;
            }
        }
    }

    return idom;
}

bool dominates(const std::vector<BlockId>& idom, BlockId a, BlockId b) {
    if (a >= idom.size() || b >= idom.size()) return false;
    if (a == kNoBlock || b == kNoBlock) return false;
    if (idom[a] == kNoBlock || idom[b] == kNoBlock) return false;
    if (a == b) return true;

    BlockId curr = b;
    while (curr != idom[curr]) {
        curr = idom[curr];
        if (curr == a) return true;
        if (curr >= idom.size() || curr == kNoBlock) break;
    }
    return false;
}

std::vector<int> loopDepths(const Function& fn) {
    if (fn.blocks.empty()) return {};

    const std::size_t numBlocks = fn.blocks.size();
    std::vector<int> depths(numBlocks, 0);

    auto idom = dominators(fn);

    std::unordered_map<BlockId, std::vector<BlockId>> header_to_latches;
    for (BlockId u = 0; u < numBlocks; ++u) {
        if (idom[u] == kNoBlock) continue;
        for (BlockId h : fn.blocks[u].succs) {
            if (h < numBlocks && dominates(idom, h, u)) {
                auto& latches = header_to_latches[h];
                if (std::find(latches.begin(), latches.end(), u) == latches.end()) {
                    latches.push_back(u);
                }
            }
        }
    }

    for (const auto& [h, latches] : header_to_latches) {
        std::vector<bool> in_loop(numBlocks, false);
        in_loop[h] = true;
        std::vector<BlockId> worklist;

        for (BlockId latch : latches) {
            if (!in_loop[latch]) {
                in_loop[latch] = true;
                worklist.push_back(latch);
            }
        }

        while (!worklist.empty()) {
            BlockId m = worklist.back();
            worklist.pop_back();

            for (BlockId p : fn.blocks[m].preds) {
                if (p < numBlocks && idom[p] != kNoBlock && !in_loop[p]) {
                    in_loop[p] = true;
                    worklist.push_back(p);
                }
            }
        }

        for (BlockId b = 0; b < numBlocks; ++b) {
            if (in_loop[b]) {
                depths[b]++;
            }
        }
    }

    return depths;
}

Liveness computeLiveness(const Function& fn) {
    if (fn.blocks.empty()) return {};

    const std::size_t numBlocks = fn.blocks.size();
    const std::size_t numValues = fn.numValues();

    Liveness result;
    result.liveIn.assign(numBlocks, BitSet(numValues));
    result.liveOut.assign(numBlocks, BitSet(numValues));

    auto rpo = reversePostorder(fn);
    if (rpo.empty()) return result;

    std::vector<BlockId> po = rpo;
    std::reverse(po.begin(), po.end());

    std::vector<bool> reachable(numBlocks, false);
    for (BlockId b : rpo) {
        reachable[b] = true;
    }

    bool changed = true;
    while (changed) {
        changed = false;
        for (BlockId b : po) {
            BitSet newLiveOut(numValues);
            for (BlockId s : fn.blocks[b].succs) {
                if (s < numBlocks && reachable[s]) {
                    BitSet edgeLive = result.liveIn[s];

                    for (const auto& in : fn.blocks[s].instrs) {
                        if (in.isPhi() && in.dst != kNoValue && in.dst < numValues) {
                            edgeLive.reset(in.dst);
                        }
                    }

                    for (const auto& in : fn.blocks[s].instrs) {
                        if (in.isPhi()) {
                            for (const auto& arg : in.phiArgs) {
                                if (arg.pred == b && arg.value != kNoValue && arg.value < numValues) {
                                    edgeLive.set(arg.value);
                                }
                            }
                        }
                    }

                    newLiveOut |= edgeLive;
                }
            }
            result.liveOut[b] = newLiveOut;

            BitSet live = result.liveOut[b];

            for (auto it = fn.blocks[b].instrs.rbegin(); it != fn.blocks[b].instrs.rend(); ++it) {
                if (!it->isPhi()) {
                    if (it->dst != kNoValue && it->dst < numValues) {
                        live.reset(it->dst);
                    }
                    for (ValueId u : it->uses) {
                        if (u != kNoValue && u < numValues) {
                            live.set(u);
                        }
                    }
                }
            }

            for (const auto& in : fn.blocks[b].instrs) {
                if (in.isPhi() && in.dst != kNoValue && in.dst < numValues) {
                    live.set(in.dst);
                }
            }

            if (live != result.liveIn[b]) {
                result.liveIn[b] = live;
                changed = true;
            }
        }
    }

    return result;
}

Interference interferenceGraph(const Function& fn) {
    const std::size_t numValues = fn.numValues();
    Graph g(numValues);
    g.setPool(&fn.values);

    for (ValueId v : fn.vregs()) {
        g.addNode(v);
    }

    if (fn.blocks.empty()) {
        return { g, 0 };
    }

    Liveness lv = computeLiveness(fn);
    auto rpo = reversePostorder(fn);

    std::size_t maxlive = 0;

    BitSet entrySet(numValues);
    for (ValueId p : fn.params) {
        if (p < numValues) {
            entrySet.set(p);
        }
    }
    BlockId entry = fn.entry();
    if (entry != kNoBlock && entry < fn.blocks.size()) {
        entrySet |= lv.liveIn[entry];
    }
    maxlive = std::max(maxlive, entrySet.count());

    for (std::size_t i = 0; i < fn.params.size(); ++i) {
        for (std::size_t j = i + 1; j < fn.params.size(); ++j) {
            ValueId p1 = fn.params[i];
            ValueId p2 = fn.params[j];
            if (p1 < numValues && p2 < numValues && p1 != p2) {
                g.addEdge(p1, p2);
            }
        }
    }

    if (entry != kNoBlock && entry < fn.blocks.size()) {
        lv.liveIn[entry].forEach([&](ValueId v) {
            for (ValueId p : fn.params) {
                if (p < numValues && p != v) {
                    g.addEdge(p, v);
                }
            }
        });
    }

    for (BlockId b : rpo) {
        maxlive = std::max(maxlive, lv.liveIn[b].count());
        maxlive = std::max(maxlive, lv.liveOut[b].count());

        BitSet live = lv.liveOut[b];

        for (auto it = fn.blocks[b].instrs.rbegin(); it != fn.blocks[b].instrs.rend(); ++it) {
            if (!it->isPhi()) {
                if (it->dst != kNoValue && it->dst < numValues) {
                    live.forEach([&](ValueId v) {
                        if (v != it->dst) {
                            g.addEdge(it->dst, v);
                        }
                    });
                }

                BitSet afterDef = live;
                if (it->dst != kNoValue && it->dst < numValues) {
                    afterDef.set(it->dst);
                }
                maxlive = std::max(maxlive, afterDef.count());

                if (it->dst != kNoValue && it->dst < numValues) {
                    live.reset(it->dst);
                }
                for (ValueId u : it->uses) {
                    if (u != kNoValue && u < numValues) {
                        live.set(u);
                    }
                }
                maxlive = std::max(maxlive, live.count());
            }
        }

        std::vector<ValueId> livePhis;
        for (const auto& in : fn.blocks[b].instrs) {
            if (in.isPhi() && in.dst != kNoValue && in.dst < numValues) {
                if (live.test(in.dst)) {
                    livePhis.push_back(in.dst);
                }
            }
        }

        for (ValueId p : livePhis) {
            live.forEach([&](ValueId v) {
                if (v != p) {
                    g.addEdge(p, v);
                }
            });
        }

        BitSet phiBoundary = live;
        for (const auto& in : fn.blocks[b].instrs) {
            if (in.isPhi() && in.dst != kNoValue && in.dst < numValues) {
                phiBoundary.set(in.dst);
            }
        }
        maxlive = std::max(maxlive, phiBoundary.count());
    }

    return { g, maxlive };
}

} // namespace ssarv
