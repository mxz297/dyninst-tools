#ifndef COVERAGE_LOCATION_OPT
#define COVERAGE_LOCATION_OPT

#include <string>
#include <unordered_map>
#include <memory>

namespace Dyninst {
    namespace PatchAPI {
        class PatchFunction;
        class PatchBlock;
        class PatchLoop;
    }
}

namespace GraphAnalysis {
    class SingleBlockGraph;
    class MultiBlockGraph;
    class MBGNode;
}

class CoverageLocationOpt {
    bool realCode;
    bool verbose;
    std::unordered_map<uint64_t, bool> instMap;
    std::unordered_map<Dyninst::PatchAPI::PatchBlock*, int> loopNestLevel;

    void determineBlocks(
        std::shared_ptr<GraphAnalysis::SingleBlockGraph>,
        std::shared_ptr<GraphAnalysis::MultiBlockGraph>,
        std::string&);

    void computeLoopNestLevels(Dyninst::PatchAPI::PatchFunction*);
    void computeLoopNestLevelsImpl(Dyninst::PatchAPI::PatchLoop* , int);
    void countInstrumentedBlocksInLoops(Dyninst::PatchAPI::PatchFunction*);

    Dyninst::PatchAPI::PatchBlock* chooseSBRep(std::shared_ptr<GraphAnalysis::MBGNode> mbgn);
    bool hasPathWithoutChild(std::shared_ptr<GraphAnalysis::SingleBlockGraph>, std::shared_ptr<GraphAnalysis::MBGNode>);
public:
    CoverageLocationOpt(Dyninst::PatchAPI::PatchFunction*, std::string, bool);
    CoverageLocationOpt(
        std::shared_ptr<GraphAnalysis::SingleBlockGraph>,
        std::shared_ptr<GraphAnalysis::MultiBlockGraph>,
        std::string);

    bool needInstrumentation(uint64_t);
};

#endif