#include "SingleBlockGraph.hpp"
#include "PatchCFG.h"

using Dyninst::PatchAPI::PatchFunction;
using Dyninst::PatchAPI::PatchBlock;

namespace GraphAnalysis {

SBGNode::SBGNode(Dyninst::PatchAPI::PatchBlock* b): block(b) {}

SingleBlockGraph::SingleBlockGraph(PatchFunction* f) {
    // Create all nodes
    for (auto b : f->blocks()) {        
        SBGNode::Ptr n = std::make_shared<SBGNode>(b);
        nodeMap.emplace(b, n);
        addNode(n);
    }

    // Add entry
    SBGNode::Ptr n = nodeMap[f->entry()];
    addEntry(n);

    // Add exits
    for (auto b : f->exitBlocks()) {
        SBGNode::Ptr n = nodeMap[b];
        addExit(n);
    }

    // Create edges
    for (auto b: f->blocks()) {
        SBGNode::Ptr n = nodeMap[b];
        for (auto e : b->targets()) {
            if (e->sinkEdge() || e->interproc()) continue;
            auto it = nodeMap.find(e->trg());
            if (it == nodeMap.end()) continue;
            n->addOutEdge(it->second);
        }
        for (auto e : b->sources()) {
            if (e->sinkEdge() || e->interproc()) continue;
            auto it = nodeMap.find(e->src());
            if (it == nodeMap.end()) continue;
            n->addInEdge(it->second);
        }
    }
}

SingleBlockGraph::Ptr SingleBlockGraph::buildDominatorGraph() {
    SingleBlockGraph::Ptr ret(new SingleBlockGraph());
    return ret;
}

}