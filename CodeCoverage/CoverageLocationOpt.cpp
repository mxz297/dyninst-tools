#include "PatchCFG.h"
#include "CoverageLocationOpt.hpp"
#include "SingleBlockGraph.hpp"
#include "MultiBlockGraph.hpp"

using Dyninst::PatchAPI::PatchFunction;
using Dyninst::PatchAPI::PatchBlock;
using GraphAnalysis::SingleBlockGraph;
using GraphAnalysis::SBGNode;
using GraphAnalysis::MultiBlockGraph;
using GraphAnalysis::MBGNode;

CoverageLocationOpt::CoverageLocationOpt(PatchFunction* f, std::string mode) {
    realCode = true;
    if (mode == "none") {
        for (auto b : f->blocks()) {
            instMap[b->start()] = true;
        }
        return;
    }
    // convert function to the graph analysis graph
    SingleBlockGraph::Ptr cfg = std::make_shared<SingleBlockGraph>(f);
    // get the super block dominator graph
    MultiBlockGraph::Ptr sbdg = std::make_shared<MultiBlockGraph>(cfg);

    determineBlocks(cfg, sbdg, mode);
}

CoverageLocationOpt::CoverageLocationOpt(SingleBlockGraph::Ptr cfg, MultiBlockGraph::Ptr sbdg, std::string mode) {
    realCode = false;
    if (mode == "none") {
        for (auto& n : sbdg->getAllNodes()) {
            MBGNode::Ptr mbgn = std::static_pointer_cast<MBGNode>(n);
            for (auto pb : mbgn->getPatchBlocks()) {
                instMap[(uint64_t)pb] = true;
            }
        }
        return;
    }
    determineBlocks(cfg, sbdg, mode);
}

bool CoverageLocationOpt::needInstrumentation(uint64_t addr) {
    return instMap.find(addr) != instMap.end();
}

void CoverageLocationOpt::determineBlocks(SingleBlockGraph::Ptr cfg, MultiBlockGraph::Ptr sbdg, std::string& mode) {
    std::set<MBGNode::Ptr> exitNodes;
    for (auto& n : sbdg->getExits()) {
        MBGNode::Ptr mbgn = std::static_pointer_cast<MBGNode>(n);
        exitNodes.insert(mbgn);
        PatchBlock* instB = chooseSBRep(mbgn);
        uint64_t addr = realCode ? instB->start() : ((uint64_t)instB);
        instMap[addr] = true;
    }
    if (mode == "leaf") return;
    for (auto & n : sbdg->getAllNodes()) {
        MBGNode::Ptr mbgn = std::static_pointer_cast<MBGNode>(n);
        if (exitNodes.find(mbgn) != exitNodes.end()) continue;
        if (hasPathWithoutChild(cfg, mbgn)) {
            PatchBlock* instB = chooseSBRep(mbgn);
            uint64_t addr = realCode ? instB->start() : ((uint64_t)instB);
            instMap[addr] = true;
        }
    }
}

PatchBlock* CoverageLocationOpt::chooseSBRep(MBGNode::Ptr mbgn) {
    // TODO: add loop analysis
    return *(mbgn->getPatchBlocks().begin());
}

static void initializeVisited(std::set<PatchBlock*> &visited, MBGNode::Ptr mbgn) {
    for (auto &n : mbgn->outEdgeList()) {
        MBGNode::Ptr m = std::static_pointer_cast<MBGNode>(n);
        for (auto pb : m->getPatchBlocks()) {
            visited.insert(pb);
        }
    }
}

static bool canReachEntry(PatchBlock* cur, SingleBlockGraph::Ptr cfg, std::set<PatchBlock*> &visited) {
    if (visited.find(cur) != visited.end()) return false;
    visited.insert(cur);
    SBGNode::Ptr c = cfg->lookupNode(cur);
    if (c->inEdgeList().empty()) return true;
    bool ret = false;
    for (auto &n : c->inEdgeList()) {
        SBGNode::Ptr pred = std::static_pointer_cast<SBGNode>(n);
        ret |= canReachEntry(pred->getPatchBlock(), cfg, visited);
    }
    return ret;
}

static bool canReachExit(PatchBlock* cur, SingleBlockGraph::Ptr cfg, std::set<PatchBlock*> &visited) {
    if (visited.find(cur) != visited.end()) return false;
    visited.insert(cur);
    SBGNode::Ptr c = cfg->lookupNode(cur);
    if (c->outEdgeList().empty()) return true;
    bool ret = false;
    for (auto &n : c->outEdgeList()) {
        SBGNode::Ptr next = std::static_pointer_cast<SBGNode>(n);
        ret |= canReachExit(next->getPatchBlock(), cfg, visited);
    }
    return ret;
}

bool CoverageLocationOpt::hasPathWithoutChild(SingleBlockGraph::Ptr cfg, MBGNode::Ptr mbgn) {
    std::set<PatchBlock*> visited;
    PatchBlock* instB = chooseSBRep(mbgn);
    initializeVisited(visited, mbgn);
    if (!canReachEntry(instB, cfg, visited)) return false;
    visited.clear();
    initializeVisited(visited, mbgn);
    if (!canReachExit(instB, cfg, visited)) return false;
    return true;
}