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
    if (mode == "none") {
        for (auto b : f->blocks()) {
            instMap[b->start()] = true;
        }        
    }
    if (mode == "leaf") {
        // convert function to the graph analysis graph
        SingleBlockGraph::Ptr cfg = std::make_shared<SingleBlockGraph>(f);
        // get the super block dominator graph
        MultiBlockGraph::Ptr sbdg = std::make_shared<MultiBlockGraph>(cfg);
        for (auto n : sbdg->getExits()) {
            MBGNode::Ptr mbgn = std::static_pointer_cast<MBGNode>(n);
            // TODO: add loop analysis
            PatchBlock* instB = *(mbgn->getPatchBlocks().begin());
            instMap[instB->start()] = true;
        }
    }
}

bool CoverageLocationOpt::needInstrumentation(PatchBlock* b) {
    return instMap.find(b->start()) != instMap.end();
}
