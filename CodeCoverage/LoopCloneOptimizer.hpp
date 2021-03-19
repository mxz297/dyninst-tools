#ifndef LOOP_CLONE_OPTIMIZER_HPP
#define LOOP_CLONE_OPTIMIZER_HPP

#include <unordered_map>

class BPatch_function;
class BPatch_basicBlock;

namespace Dyninst {
    namespace PatchAPI {
        class PatchLoop;
    }
}

#include "Snippet.h"
#include "CoverageSnippet.hpp"

class LoopCloneOptimizer {
    Dyninst::PatchAPI::PatchFunction* f;
    std::set<BPatch_basicBlock*> &instBlocks;

    std::unordered_map<uint64_t, CoverageSnippet::Ptr> snippetMap;
    std::unordered_map<uint64_t, Dyninst::PatchAPI::PatchBlock*> origBlockMap;
    std::vector< std::map<Dyninst::PatchAPI::PatchBlock*, Dyninst::PatchAPI::PatchBlock* > > versionedCloneMap;

    void cloneALoop(Dyninst::PatchAPI::PatchLoop *l);
    void makeOneCopy(int, vector<Dyninst::PatchAPI::PatchBlock*> &);

public:
    LoopCloneOptimizer(BPatch_function*, std::set<BPatch_basicBlock*> &);
    void instrument();
};

#endif