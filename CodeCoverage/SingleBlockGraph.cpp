#include "SingleBlockGraph.hpp"
#include "PatchCFG.h"

using Dyninst::PatchAPI::PatchFunction;
using Dyninst::PatchAPI::PatchBlock;

namespace GraphAnalysis {

SBGNode::SBGNode(Dyninst::PatchAPI::PatchBlock* b): block(b) {}

bool SBGNode::dominates(SBGNode::Ptr b) {
    if (getPatchBlock() == b->getPatchBlock()) return true;
    for (auto& im : immDominates) {
        if (im->dominates(b)) return true;
    }
    return false;
}

void SBGNode::PrintNodeData() {
    printf("SBGNode<%p>", block);
}

SingleBlockGraph::SingleBlockGraph(PatchFunction* f) {
    // Create all nodes
    for (auto b : f->blocks()) {
        SBGNode::Ptr n = std::make_shared<SBGNode>(b);
        nodeMap.emplace(b, n);
        addSBGNode(n);
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
    // Create an empty graph
    SingleBlockGraph::Ptr ret(new SingleBlockGraph());

    // Create new nodes and the mapping between the new graph and the old graph
    std::unordered_map<SBGNode::Ptr, SBGNode::Ptr> nodeMap;
    for (const auto& n : allNodes) {
        SBGNode::Ptr sbgn = std::static_pointer_cast<SBGNode>(n);
        SBGNode::Ptr newSBGN = std::make_shared<SBGNode>(sbgn->getPatchBlock());
        nodeMap.emplace(sbgn, newSBGN);
        ret->addSBGNode(newSBGN);        
    }

    // Add entry
    for (const auto & n : entries) {
        SBGNode::Ptr sbgn = std::static_pointer_cast<SBGNode>(n);
        ret->addEntry(nodeMap[sbgn]);
    }

    // Add exits
    for (const auto & n : exits) {
        SBGNode::Ptr sbgn = std::static_pointer_cast<SBGNode>(n);
        ret->addExit(nodeMap[sbgn]);
    }

    // Add edges based on dominator tree
    Graph::EdgeList elist;
    dominatorTree(elist);
    for (const auto& edge : elist) {
        SBGNode::Ptr source = std::static_pointer_cast<SBGNode>(edge.first);
        SBGNode::Ptr target = std::static_pointer_cast<SBGNode>(edge.second);
        nodeMap[source]->addOutEdge(nodeMap[target]);
        nodeMap[target]->addInEdge(nodeMap[source]);

        source->immDominates.insert(target);
    }

    // Add edges based on post dominator tree
    elist.clear();
    postDominatorTree(elist);
    for (const auto& edge : elist) {
        SBGNode::Ptr source = std::static_pointer_cast<SBGNode>(edge.first);
        SBGNode::Ptr target = std::static_pointer_cast<SBGNode>(edge.second);
        nodeMap[source]->addOutEdge(nodeMap[target]);
        nodeMap[target]->addInEdge(nodeMap[source]);
    }

    return ret;
}

SBGNode::Ptr SingleBlockGraph::lookupNode(Dyninst::PatchAPI::PatchBlock* b) {
    return nodeMap[b];
}

void SingleBlockGraph::addSBGNode(SBGNode::Ptr n) {
    nodeMap[n->getPatchBlock()] = n;
    Graph::addNode(n);
}

}