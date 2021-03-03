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
    Dyninst::PatchAPI::PatchBlock* block;
public:
    using Ptr = std::shared_ptr<SBGNode>;
    SBGNode(Dyninst::PatchAPI::PatchBlock*);
};

class SingleBlockGraph : public Graph {
    std::unordered_map<Dyninst::PatchAPI::PatchBlock*, SBGNode::Ptr> nodeMap;
    SingleBlockGraph() {}
public:
    using Ptr = std::shared_ptr<SingleBlockGraph>;    
    SingleBlockGraph(Dyninst::PatchAPI::PatchFunction*);
    Ptr buildDominatorGraph();
};

}