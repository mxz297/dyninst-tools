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

class SingleBlockGraph;

class MBGNode: public Node {
    std::set<Dyninst::PatchAPI::PatchBlock*> blocks;
public:
    using Ptr = std::shared_ptr<MBGNode>;
    MBGNode();
    void addPatchBlock(Dyninst::PatchAPI::PatchBlock* b) { blocks.insert(b); }
    const std::set<Dyninst::PatchAPI::PatchBlock*>& getPatchBlocks() { return blocks; }
    bool hasSCDGEdge(Ptr, std::shared_ptr<SingleBlockGraph>);
};

class MultiBlockGraph : public Graph {
    //std::unordered_map<Dyninst::PatchAPI::PatchBlock*, MBGNode::Ptr> nodeMap;    
public:
    using Ptr = std::shared_ptr<MultiBlockGraph>;    
    MultiBlockGraph(std::shared_ptr<SingleBlockGraph>);    
};

}