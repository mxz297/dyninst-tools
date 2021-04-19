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

const int LIMIT = 10;

extern bool threadLocalMemory;
extern BPatch_binaryEdit *binEdit;
extern int gsOffset;
extern std::string pgo_filename;

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
    BPatch_function* bf,
    std::set<BPatch_basicBlock*> & blocks
): instBlocks(blocks) {
    f = Dyninst::PatchAPI::convert(bf);    
    for (auto b : f->blocks()) {
        origBlockMap[b->start()] = b;
    }
}

void LoopCloneOptimizer::instrument() {
    for (auto b : instBlocks) {
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

    vector<PatchLoop*> loops;
    set<PatchLoop*> cloneLoops;
    set<PatchBlock*> coveredBlocks;

    f->getLoops(loops);
    for (auto l : loops) {
        bool inPGOFile = false;
        std::vector<PatchBlock*> entries;
        l->getLoopEntries(entries);
        for (auto b: entries) {
            if (loopMap.find(b->start()) != loopMap.end()) {
                inPGOFile = true;
                break;
            }
        }
        if (!inPGOFile) continue;

        int instrumentedBlockCnt = 0;
        vector<PatchBlock*> blocks;
        l->getLoopBasicBlocks(blocks);
        for (auto b : blocks) {
            if (snippetMap.find(b->start()) != snippetMap.end()) {
                instrumentedBlockCnt += 1;
            }
        }
        if (instrumentedBlockCnt > LIMIT) continue;

        bool containClonedBlocks = false;
        for (auto b : blocks) {
            if (coveredBlocks.find(b) != coveredBlocks.end()) {
                containClonedBlocks = true;
                break;
            }
        }
        if (containClonedBlocks) continue;

        for (auto b: blocks) {
            coveredBlocks.insert(b);
        }
        cloneLoops.insert(l);

        printf("Clone loop %p with entries", l);
        for (auto b : entries) {
            printf(" [%lx, %lx)", b->start(), b->end());
        }
        printf("\n");
        for (auto b : blocks) {
            printf("\t[%lx, %lx)", b->start(), b->end());
            if (snippetMap.find(b->start()) != snippetMap.end()) printf(", instrumented\n"); else printf("\n");
        }
    }

    for (auto l : cloneLoops) {
        cloneALoop(l);
    }
}

void LoopCloneOptimizer::cloneALoop(PatchLoop *l) {
    vector<PatchBlock*> blocks;
    l->getLoopBasicBlocks(blocks);
    vector<PatchBlock*> instumentedBlocks;
    for (auto b : blocks) {
        if (snippetMap.find(b->start()) != snippetMap.end()) {
            instumentedBlocks.emplace_back(b);
        }
    }
    if (instumentedBlocks.empty()) return;
    versionedCloneMap.clear();

    int cloneCopies = 1 << instumentedBlocks.size();

    for (int i = 1; i < cloneCopies; ++i) {
        makeOneCopy(i, blocks);
    }

    for (int i = 0; i < cloneCopies; ++i) {
        int index = 0;
        for (auto b : instumentedBlocks) {
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
        for (auto b : instumentedBlocks) {
            if (i & (1 << index)) {
                index++;
                continue;
            }
            int newVersion = i | (1 << index);
            PatchBlock* instB = versionedCloneMap[newVersion - 1][b];
            Point* p = patcher->findPoint(Dyninst::PatchAPI::Location::BlockInstance(f, instB), Point::BlockEntry);
            assert(p);
            CoverageSnippet::Ptr snippet = snippetMap[b->start()];
            p->pushBack(snippet);
            index += 1;
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

std::map<uint64_t, int> LoopCloneOptimizer::loopMap;

#include <iostream>
#include <fstream>

bool LoopCloneOptimizer::readPGOFile(const std::string& filename) {
    std::ifstream infile(filename, std::fstream::in);
    uint64_t loopAddr;
    double metric;
    int order = 0;
    while (infile >> std::hex >> loopAddr >> metric) {
        order += 1;
        loopMap[loopAddr] = order;
    }
    return true;
}