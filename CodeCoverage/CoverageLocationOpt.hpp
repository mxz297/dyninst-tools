#ifndef COVERAGE_LOCATION_OPT
#define COVERAGE_LOCATION_OPT

#include <string>
#include <unordered_map>
#include <memory>

namespace Dyninst {
    namespace PatchAPI {
        class PatchFunction;
        class PatchBlock;
    }
}

namespace GraphAnalysis {
    class SingleBlockGraph;
    class MultiBlockGraph;
    class MBGNode;
}

class CoverageLocationOpt {
    bool realCode;
    std::unordered_map<uint64_t, bool> instMap;

    void determineBlocks(
        std::shared_ptr<GraphAnalysis::SingleBlockGraph>,
        std::shared_ptr<GraphAnalysis::MultiBlockGraph>,
        std::string&);

    Dyninst::PatchAPI::PatchBlock* chooseSBRep(std::shared_ptr<GraphAnalysis::MBGNode> mbgn);
    bool hasPathWithoutChild(std::shared_ptr<GraphAnalysis::SingleBlockGraph>, std::shared_ptr<GraphAnalysis::MBGNode>);
public:
    CoverageLocationOpt(Dyninst::PatchAPI::PatchFunction*, std::string);
    CoverageLocationOpt(
        std::shared_ptr<GraphAnalysis::SingleBlockGraph>,
        std::shared_ptr<GraphAnalysis::MultiBlockGraph>,
        std::string);

    bool needInstrumentation(uint64_t);
};

#endif