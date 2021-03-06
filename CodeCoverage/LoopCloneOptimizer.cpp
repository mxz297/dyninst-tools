#include "LoopCloneOptimizer.hpp"
#include "CoverageSnippet.hpp"

#include "BPatch.h"
#include "BPatch_binaryEdit.h"
#include "BPatch_function.h"
#include "BPatch_basicBlock.h"
#include "BPatch_point.h"

#include "PatchCFG.h"
#include "Point.h"
#include "PatchModifier.h"
#include "PatchMgr.h"

#include "slicing.h"
#include "Graph.h"

extern bool threadLocalMemory;
extern BPatch_binaryEdit *binEdit;
extern int gsOffset;
extern std::string pgo_filename;
extern int loop_clone_limit;
extern double pgo_ratio;

struct PGOBlock {
    uint64_t addr;
    double metric;
    PGOBlock(uint64_t a, double m): addr(a), metric(m) {}
};

static std::vector<PGOBlock> pgoBlocks;
static double totalMetrics = 0.0;

using Dyninst::PatchAPI::Snippet;
using Dyninst::PatchAPI::Point;
using Dyninst::PatchAPI::PatchLoop;
using Dyninst::PatchAPI::PatchFunction;
using Dyninst::PatchAPI::PatchBlock;
using Dyninst::PatchAPI::PatchEdge;
using Dyninst::PatchAPI::PatchModifier;
using Dyninst::PatchAPI::PatchMgr;
using Dyninst::PatchAPI::CFGMaker;
using Dyninst::Address;

static int combineVersionNumber(int loopV, int inlineV) {
    return (loopV << 16) + inlineV;
}

static int getInlineVersionNumber(PatchBlock* b) {
    return b->getCloneVersion() & 0xFFFF;
}

static bool CheckJumpTableOutside(PatchFunction* f, PatchLoop* l) {
    vector<PatchBlock*> blocks;
    l->getLoopBasicBlocks(blocks);

    std::set<Address> loopBlockAddr;
    for (auto b: blocks) {
        loopBlockAddr.insert(b->start());
    }

    for (auto b : blocks) {
        const auto jit = f->getJumpTableMap().find(b);
        if (jit == f->getJumpTableMap().end()) continue;
        const PatchFunction::PatchJumpTableInstance& pjti = jit->second;

        Dyninst::Graph::Ptr g = pjti.formatSlice;
        Dyninst::NodeIterator nbegin, nend;
        g->allNodes(nbegin, nend);
        for (; nbegin != nend; ++nbegin) {
            if ((*nbegin)->addr() == 0) continue;
            Dyninst::SliceNode::Ptr cur = boost::static_pointer_cast<Dyninst::SliceNode>(*nbegin);
            Address blockStart = cur->block()->start();
            if (loopBlockAddr.find(blockStart) == loopBlockAddr.end()) {
                return true;
            }
        }
    }
    return false;
}

static CoverageSnippet::Ptr getCoverageSnippet(Dyninst::Address addr) {
    return boost::static_pointer_cast<CoverageSnippet>(
                threadLocalMemory ?
                ThreadLocalMemCoverageSnippet::create(new ThreadLocalMemCoverageSnippet(addr)) :
                GlobalMemCoverageSnippet::create(new GlobalMemCoverageSnippet(addr)));
}


LoopCloneOptimizer::LoopCloneOptimizer(
    std::map<PatchFunction*, std::set<PatchBlock*> > & blocks,
    std::vector<PatchFunction*>& fs
): instBlocks(blocks), funcs(fs) {
    std::set<PatchLoop*> containJumpTableOutside;
    for (auto & mapIt : instBlocks) {
        PatchFunction* f = mapIt.first;
        for (auto b : f->blocks()) {
            origBlockMap[b->start()].emplace_back(b);
        }
        std::vector<PatchLoop*> loops;

        // Loops here will include inlined callees
        f->getOuterLoops(loops);
        for (auto l : loops) {
            if (CheckJumpTableOutside(f, l)) {
                containJumpTableOutside.insert(l);
            }
            vector<PatchBlock*> blocks;
            l->getLoopBasicBlocks(blocks);
            int size = 0;
            for (auto b : blocks) {
                origBlockLoopMap[b] = l;
                size += b->end() - b->start();
            }
            loopSizeMap[l] = size;
        }
    }

    double optimized_metrics = 0;
    std::map<PatchLoop*, int> loopInstCount;
    for (auto& pgoBlock : pgoBlocks) {
        if (origBlockMap.find(pgoBlock.addr) == origBlockMap.end()) continue;
        bool makeClone = false;
        for (auto b : origBlockMap[pgoBlock.addr]) {
            printf("Examine block [%lx, %lx)", b->start(), b->end());
            PatchLoop * l = origBlockLoopMap[b];
            if (l == nullptr) {
                printf(", skip due to not in any loop\n");
                continue;
            }
            if (containJumpTableOutside.find(l) != containJumpTableOutside.end()) {
                printf(", skip due to jump table computation outside loop\n");
                continue;
            }
            if (loopInstCount[l] < loop_clone_limit) {
                loopInstCount[l] += 1;
                printf(", ok to clone, loop size %d\n", loopSizeMap[l]);
                blocksToClone.insert(b);
                makeClone = true;
            } else {
                printf(", skip due to reaching loop clone limit\n");
            }
        }
        if (makeClone) optimized_metrics += pgoBlock.metric;
        if (optimized_metrics > pgo_ratio * totalMetrics) break;
    }
    printf("Optimize %.2lf percent metrics\n", optimized_metrics * 100.0 / totalMetrics);
}

void LoopCloneOptimizer::instrument() {
    for (auto f : funcs) {
        f->markModified();
        instrumentedBlocks.clear();
        doLoopClone(f);

        // After doing loop cloning,
        // need to instrument blocks outside any cloned loops
        for (auto b : instBlocks[f]) {
            if (instrumentedBlocks.find(b) != instrumentedBlocks.end()) continue;            
            PatchMgr::Ptr mgr = f->obj()->mgr();
            auto p = mgr->findPoint(
                Dyninst::PatchAPI::Location::BlockInstance(f, b, true),
                Point::BlockEntry,
                true
            );
            assert(p != nullptr);
            p->pushBack(getCoverageSnippet(b->start()));
        }
    }
}

void LoopCloneOptimizer::doLoopClone(PatchFunction* f) {
    set<PatchLoop*> cloneLoops;
    for (auto b : f->blocks()) {
        if (blocksToClone.find(b) == blocksToClone.end()) continue;
        cloneLoops.insert(origBlockLoopMap[b]);
    }

    for (auto l : cloneLoops) {
        cloneALoop(f, l);
    }
}

void LoopCloneOptimizer::cloneALoop(PatchFunction* f, PatchLoop *l) {
    vector<PatchBlock*> blocks;
    l->getLoopBasicBlocks(blocks);
    set<PatchBlock*> loopInstumentedBlocks;
    set<PatchBlock*> clonedBlocks;

    std::set<PatchBlock*>& funcInstumentedBlocks = instBlocks[f];

    std::map<PatchBlock*, PatchBlock*> cloneBlockMap;

    std::map< std::pair<uint64_t, int> , PatchBlock*> blockBeforeCloneMap;
    for (auto b : blocks) {
        cloneBlockMap[b] = b;
        if (funcInstumentedBlocks.find(b) != funcInstumentedBlocks.end()) {
            loopInstumentedBlocks.insert(b);
            // Only make loop clones for instrumented blocks.
            // It is possible that blocks that corresponding high cost blocks
            // in profile are no instrumented due to inlining.
            if (blocksToClone.find(b) != blocksToClone.end()) {
                clonedBlocks.insert(b);
            }
        }
        blockBeforeCloneMap[std::make_pair(b->start(), b->getCloneVersion())] = b;
    }

    if (clonedBlocks.empty()) return;
    versionedCloneMap.clear();
    versionedCloneMap.emplace_back(cloneBlockMap);

    int cloneCopies = 1 << clonedBlocks.size();

    for (int i = 1; i < cloneCopies; ++i) {
        makeOneCopy(f, i, blocks);
    }

    assert(cloneCopies == versionedCloneMap.size());

    // Redirect edges among clones
    for (int i = 0; i < cloneCopies; ++i) {
        int index = 0;
        for (auto b : clonedBlocks) {
            if (i & (1 << index)) {
                index++;
                continue;
            }
            if (i > 0) {
                b = versionedCloneMap[i][b];
            }
            int newVersion = i | (1 << index);
            for (auto e : b->targets()) {
                if (e->type() == Dyninst::ParseAPI::CATCH) continue;
                if (e->sinkEdge() || e->interproc()) continue;
                int inlineVersion = getInlineVersionNumber(e->trg());
                PatchBlock* origB = blockBeforeCloneMap[std::make_pair(e->trg()->start(), inlineVersion)];
                auto it = versionedCloneMap[newVersion].find(origB);
                if (it == versionedCloneMap[newVersion].end()) continue;
                PatchBlock* newTarget = it->second;
                assert(PatchModifier::redirect(e, newTarget));
            }
            index += 1;
        }
    }

    // Insert coverage snippet
    // Note that we must insert coverage snippet after cloning blocks
    // because block cloning will copy snippets, which is necessary for inlining
    PatchMgr::Ptr patcher = Dyninst::PatchAPI::convert(binEdit);
    for (int i = 0; i < cloneCopies; ++i) {
        int index = 0;
        for (auto b : clonedBlocks) {
            if (i & (1 << index)) {
                index++;
                continue;
            }
            PatchBlock* instB = versionedCloneMap[i][b];
            Point* p = patcher->findPoint(Dyninst::PatchAPI::Location::BlockInstance(f, instB), Point::BlockEntry);
            assert(p);
            CoverageSnippet::Ptr snippet = getCoverageSnippet(b->start());
            instrumentedBlocks.insert(b);
            assert(snippet != nullptr);
            p->pushBack(snippet);
            index += 1;
        }
        for (auto b : loopInstumentedBlocks) {
            if (clonedBlocks.find(b) != clonedBlocks.end()) continue;
            PatchBlock* instB = versionedCloneMap[i][b];
            Point* p = patcher->findPoint(Dyninst::PatchAPI::Location::BlockInstance(f, instB), Point::BlockEntry);
            assert(p);
            CoverageSnippet::Ptr snippet = getCoverageSnippet(b->start());
            instrumentedBlocks.insert(b);
            assert(snippet != nullptr);
            p->pushBack(snippet);
        }
    }
}

void LoopCloneOptimizer::makeOneCopy(PatchFunction* f, int version, vector<PatchBlock*> &blocks) {
    // Clone all blocks in the loop
    std::map<PatchBlock*, PatchBlock*> cloneBlockMap;
    std::vector<PatchBlock*> newBlocks;
    CFGMaker* cfgMaker = BPatch::getBPatch()->getCFGMaker();
    for (auto b : blocks) {
        PatchBlock* cloneB = cfgMaker->cloneBlock(b, b->object());
        uint64_t v = combineVersionNumber(version, getInlineVersionNumber(b));
        cloneB->setCloneVersion(v);
        cloneBlockMap[b] = cloneB;
        newBlocks.emplace_back(cloneB);
        PatchModifier::addBlockToFunction(f, cloneB);
        cloneB->cloneInstrumentation(f, f, b);
    }

    versionedCloneMap.emplace_back(cloneBlockMap);

    // 3. Copy the jump table data from calee to caller
    // Jump table data copy should be done before redirecting edges
    for (auto &it : cloneBlockMap) {
        PatchBlock* origB = it.first;
        PatchBlock* newB = it.second;
        const auto jit = f->getJumpTableMap().find(origB);
        if (jit == f->getJumpTableMap().end()) continue;
        PatchFunction::PatchJumpTableInstance pjti = jit->second;

        // Build a map from target address to PatchEdge
        // We can use this map to match cloned edge with original edge
        std::unordered_map<Dyninst::Address, PatchEdge*> newEdges;
        for (auto e : newB->targets()) {
            if (e->sinkEdge() || e->type() != Dyninst::ParseAPI::INDIRECT) continue;
            newEdges[e->trg()->start()] = e;
        }

        // Update edges in jump table data to cloned edges
        for (auto& e : pjti.tableEntryEdges) {
            Dyninst::Address trg = e->trg()->start();
            auto edgeIter = newEdges.find(trg);
            assert(edgeIter != newEdges.end());
            e = edgeIter->second;
        }

        f->addJumpTableInstance(newB, pjti);
    }

    // 4. The edges of the cloned blocks are still from/to original blocks
    // Now we redirect all edges
    for (auto b : newBlocks) {
        for (auto e : b->targets()) {
            if (e->sinkEdge() || e->interproc()) continue;
            if (e->type() == Dyninst::ParseAPI::CATCH) continue;
            auto it = cloneBlockMap.find(e->trg());
            if (it == cloneBlockMap.end()) continue;
            PatchBlock* newTarget = it->second;
            assert(PatchModifier::redirect(e, newTarget));
        }
    }
}

#include <iostream>
#include <fstream>

bool LoopCloneOptimizer::readPGOFile(const std::string& filename) {
    std::ifstream infile(filename, std::fstream::in);
    uint64_t addr;
    double metric;
    while (infile >> std::hex >> addr >> metric) {
        addr -= 1;
        pgoBlocks.emplace_back(PGOBlock(addr, metric));
        totalMetrics += metric;
    }
    std::sort(pgoBlocks.begin(), pgoBlocks.end(),
        [] (const PGOBlock& a, const PGOBlock& b) {
            return a.metric > b.metric;
        }
    );
    return true;
}