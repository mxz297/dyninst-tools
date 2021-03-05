#include "MultiBlockGraph.hpp"
#include "SingleBlockGraph.hpp"

#include "PatchCFG.h"

using Dyninst::PatchAPI::PatchFunction;
using Dyninst::PatchAPI::PatchBlock;


namespace GraphAnalysis {

bool MBGNode::hasSCDGEdge(MBGNode::Ptr target, SingleBlockGraph::Ptr cfg) {
    for (auto & pb1: getPatchBlocks()) {
        SBGNode::Ptr n1 = cfg->lookupNode(pb1);
        for (auto & pb2: target->getPatchBlocks()) {
            SBGNode::Ptr n2 = cfg->lookupNode(pb2);
            if (n1->dominates(n2)) return true;
        }
    }
    return false;
}

MultiBlockGraph::MultiBlockGraph(SingleBlockGraph::Ptr cfg) {
    //Build the dominator graph and fill in domination informaiton in cfg
    SingleBlockGraph::Ptr dominatorGraph = cfg->buildDominatorGraph();

    std::vector< std::set<Node::Ptr> > sccList;
    dominatorGraph->SCC(sccList);

    // Create new nodes
    for (const auto& scc : sccList) {
        MBGNode::Ptr mbgn = std::make_shared();
        for (const auto& node : scc) {
            const SBGNode::Ptr sbgn = static_pointer_cast<SBGNode>(node);
            mbgn->addPatchBlock(sbgn->getPatchBlock());
        }
        addNode(mbgn);
    }

    // Create new edges
    for (auto & n1 : allNodes) {
        MBGNode::Ptr mbgn1 = static_pointer_cast<MBGNode>(n1);
        for (auto & n2: allNodes) {
            MBGNode::Ptr mbgn2 = static_pointer_cast<MBGNode>(n2);
            if (mbgn1 == mbgn2) continue;
            if (mbgn1->hasSCDGEdge(mbgn2, cfg)) {
                mbgn1->addOutEdge(mbgn2);
            }
        }
    }

    // Mark entry and exit
    for (auto & n : allNodes) {
        if (n->inEdgeList().empty()) addEntry(n);
        if (n->outEdgeList().empty()) addExit(n);
    }
}

}