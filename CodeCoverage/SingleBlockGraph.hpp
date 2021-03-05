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
    bool dominates(SBGNode::Ptr);
private:
    Dyninst::PatchAPI::PatchBlock* block;    
    std::set<SBGNode::Ptr> immDominates;    
};

class SingleBlockGraph : public Graph {
    std::unordered_map<Dyninst::PatchAPI::PatchBlock*, SBGNode::Ptr> nodeMap;
    SingleBlockGraph() {}
public:
    using Ptr = std::shared_ptr<SingleBlockGraph>;    
    SingleBlockGraph(Dyninst::PatchAPI::PatchFunction*);
    Ptr buildDominatorGraph();
    SBGNode::Ptr lookupNode(Dyninst::PatchAPI::PatchBlock*);
};

}