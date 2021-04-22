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

LoopCloneOptimizer::LoopCloneOptimizer(
    std::map<BPatch_function*, std::set<BPatch_basicBlock*> > & blocks
): instBlocks(blocks) {    
    for (auto & mapIt : instBlocks) {
        f = Dyninst::PatchAPI::convert(mapIt.first);
        for (auto b : f->blocks()) {
            origBlockMap[b->start()] = b;
        }
        std::vector<PatchLoop*> loops;
        f->getOuterLoops(loops);
        for (auto l : loops) {
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
        PatchBlock* b = origBlockMap[pgoBlock.addr];        
        assert(b != nullptr);
        printf("Examine block [%lx, %lx)", b->start(), b->end());
        PatchLoop * l = origBlockLoopMap[b];
        if (l == nullptr) {
            printf(", skip due to not in any loop\n");
            continue;
        }
        if (loopInstCount[l] < loop_clone_limit) {
            loopInstCount[l] += 1;
            printf(", ok to clone, loop size %d\n", loopSizeMap[l]);
            optimized_metrics += pgoBlock.metric;
            blocksToClone.insert(b);
        } else {
            printf(", skip doe to reach loop clone limit\n");
        }
        if (optimized_metrics > pgo_ratio * totalMetrics) break;
    }
    printf("Optimize %.2lf percent metrics\n", optimized_metrics * 100.0 / totalMetrics);  
}

void LoopCloneOptimizer::instrument() {
    for (auto & mapIt : instBlocks) {
        f = Dyninst::PatchAPI::convert(mapIt.first);   
        for (auto b : mapIt.second) {
            CoverageSnippet::Ptr coverage = boost::static_pointer_cast<CoverageSnippet>(
                threadLocalMemory ?
                ThreadLocalMemCoverageSnippet::create(new ThreadLocalMemCoverageSnippet()) :
                GlobalMemCoverageSnippet::create(new GlobalMemCoverageSnippet()));

            BPatch_point* bp = b->findEntryPoint();
            assert(bp != nullptr);

            Point* p = Dyninst::PatchAPI::convert(bp, BPatch_callBefore);
            assert(p != nullptr);
            snippetMap[b->getStartAddress()] = coverage;
            p->pushBack(coverage);
        }
        doLoopClone(f);
    }
}

void LoopCloneOptimizer::doLoopClone(PatchFunction* f) {
    set<PatchLoop*> cloneLoops;
    for (auto b : f->blocks()) {
        if (blocksToClone.find(b) == blocksToClone.end()) continue;
        cloneLoops.insert(origBlockLoopMap[b]);
    }
    
    for (auto l : cloneLoops) {
        cloneALoop(l);
    }        
}

void LoopCloneOptimizer::cloneALoop(PatchLoop *l) {
    vector<PatchBlock*> blocks;
    l->getLoopBasicBlocks(blocks);
    set<PatchBlock*> instumentedBlocks;
    set<PatchBlock*> clonedBlocks;
    for (auto b : blocks) {
        if (snippetMap.find(b->start()) != snippetMap.end()) {
            instumentedBlocks.insert(b);
        }
        if (blocksToClone.find(b) != blocksToClone.end()) {
            clonedBlocks.insert(b);
        }
    }
    if (clonedBlocks.empty()) return;
    versionedCloneMap.clear();

    int cloneCopies = 1 << clonedBlocks.size();

    for (int i = 1; i < cloneCopies; ++i) {
        makeOneCopy(i, blocks);
    }

    for (int i = 0; i < cloneCopies; ++i) {
        int index = 0;
        for (auto b : clonedBlocks) {
            if (i & (1 << index)) {
                index++;
                continue;
            }
            if (i > 0) {
                b = versionedCloneMap[i - 1][b];
            }
            int newVersion = i | (1 << index);
            for (auto e : b->targets()) {
                if (e->type() == Dyninst::ParseAPI::CATCH) continue;
                if (e->sinkEdge() || e->interproc()) continue;
                PatchBlock* origB = origBlockMap[e->trg()->start()];
                auto it = versionedCloneMap[newVersion - 1].find(origB);
                if (it == versionedCloneMap[newVersion - 1].end()) continue;
                PatchBlock* newTarget = it->second;
                assert(PatchModifier::redirect(e, newTarget));
            }
            index += 1;
        }
    }

    PatchMgr::Ptr patcher = Dyninst::PatchAPI::convert(binEdit);
    for (int i = 1; i < cloneCopies; ++i) {
        int index = 0;
        for (auto b : clonedBlocks) {
            if (i & (1 << index)) {
                index++;
                continue;
            }            
            PatchBlock* instB = versionedCloneMap[i - 1][b];
            Point* p = patcher->findPoint(Dyninst::PatchAPI::Location::BlockInstance(f, instB), Point::BlockEntry);
            assert(p);
            CoverageSnippet::Ptr snippet = snippetMap[b->start()];
            p->pushBack(snippet);
            index += 1;
        }
        for (auto b : instumentedBlocks) {
            if (clonedBlocks.find(b) != clonedBlocks.end()) continue;
            PatchBlock* instB = versionedCloneMap[i - 1][b];
            Point* p = patcher->findPoint(Dyninst::PatchAPI::Location::BlockInstance(f, instB), Point::BlockEntry);
            assert(p);
            CoverageSnippet::Ptr snippet = snippetMap[b->start()];
            p->pushBack(snippet);            
        }
    }
}

void LoopCloneOptimizer::makeOneCopy(int version, vector<PatchBlock*> &blocks) {
    // Clone all blocks in the loop
    std::map<PatchBlock*, PatchBlock*> cloneBlockMap;
    std::vector<PatchBlock*> newBlocks;
    CFGMaker* cfgMaker = BPatch::getBPatch()->getCFGMaker();
    for (auto b : blocks) {
        PatchBlock* cloneB = cfgMaker->cloneBlock(b, b->object());
        cloneB->setCloneVersion(version);
        cloneBlockMap[b] = cloneB;
        newBlocks.emplace_back(cloneB);
        PatchModifier::addBlockToFunction(f, cloneB);
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