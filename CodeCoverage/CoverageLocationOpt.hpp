#ifndef COVERAGE_LOCATION_OPT
#define COVERAGE_LOCATION_OPT

#include <string>
#include <unordered_map>

namespace Dyninst {
    namespace PatchAPI {
        class PatchFunction;
        class PatchBlock;
    }
}

class CoverageLocationOpt {
    std::unordered_map<uint64_t, bool> instMap;
public:
    CoverageLocationOpt(Dyninst::PatchAPI::PatchFunction*, std::string);
    bool needInstrumentation(Dyninst::PatchAPI::PatchBlock*);
};

#endif