#ifndef SINGLE_BLOCK_GRAPH
#define SINGLE_BLOCK_GRAPH

#include "Graph.hpp"

#include <memory>
#include <unordered_map>

namespace Dyninst{
    namespace PatchAPI{
        class PatchFunction;
        class PatchBlock;
    }
}

namespace GraphAnalysis {

class SBGNode: public Node {
    friend class SingleBlockGraph;
public:
    using Ptr = std::shared_ptr<SBGNode>;
    SBGNode(Dyninst::PatchAPI::PatchBlock*);
    Dyninst::PatchAPI::PatchBlock* getPatchBlock() { return block; }
    virtual void PrintNodeData(bool) override;
private:
    Dyninst::PatchAPI::PatchBlock* block;
};

class SingleBlockGraph : public Graph {
    std::unordered_map<Dyninst::PatchAPI::PatchBlock*, SBGNode::Ptr> nodeMap;
    static Dyninst::PatchAPI::PatchBlock* sinkBlock;
public:
    using Ptr = std::shared_ptr<SingleBlockGraph>;
    SingleBlockGraph(Dyninst::PatchAPI::PatchFunction*);
    SingleBlockGraph() {}
    Ptr buildDominatorGraph();
    SBGNode::Ptr lookupNode(Dyninst::PatchAPI::PatchBlock*);
    void addSBGNode(SBGNode::Ptr);
};

}

#endif