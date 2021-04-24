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
    std::map<Dyninst::PatchAPI::PatchLoop*, int> loopSizeMap;
    std::map<Dyninst::PatchAPI::PatchBlock*, Dyninst::PatchAPI::PatchLoop*> origBlockLoopMap;

    std::map<BPatch_function*, std::set<BPatch_basicBlock*> > &instBlocks;
    std::vector<BPatch_function*>& funcs;

    std::unordered_map<uint64_t, CoverageSnippet::Ptr> snippetMap;
    std::unordered_map<uint64_t, Dyninst::PatchAPI::PatchBlock*> origBlockMap;
    std::vector< std::map<Dyninst::PatchAPI::PatchBlock*, Dyninst::PatchAPI::PatchBlock* > > versionedCloneMap;

    std::set<Dyninst::PatchAPI::PatchBlock*> blocksToClone;

    void doLoopClone(Dyninst::PatchAPI::PatchFunction* f);
    void cloneALoop(Dyninst::PatchAPI::PatchFunction*, Dyninst::PatchAPI::PatchLoop *l);
    void makeOneCopy(Dyninst::PatchAPI::PatchFunction*, int, vector<Dyninst::PatchAPI::PatchBlock*> &);


public:
    static bool readPGOFile(const std::string&);
    LoopCloneOptimizer(std::map<BPatch_function*, std::set<BPatch_basicBlock*> >&, std::vector<BPatch_function*>&);
    void instrument();
};

#endif