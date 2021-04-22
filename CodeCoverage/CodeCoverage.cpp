#include "BPatch.h"
#include "BPatch_function.h"
#include "BPatch_object.h"
#include "BPatch_flowGraph.h"
#include "BPatch_basicBlock.h"
#include "BPatch_point.h"

#include "PatchCFG.h"
#include "CFG.h"
#include "CodeSource.h"
#include "Region.h"

#include <cstring>

#include "CoverageLocationOpt.hpp"
#include "CoverageSnippet.hpp"
#include "LoopCloneOptimizer.hpp"

using namespace Dyninst;
using namespace PatchAPI;

BPatch bpatch;
BPatch_binaryEdit *binEdit;
BPatch_image* image;

bool instSharedLibs = false;
bool enableJumptableReloc = true;
bool enableFuncPointerReloc = true;
bool threadLocalMemory = true;
bool verbose = false;

int nops = 0;
int loop_clone_limit = 5;

double pgo_ratio = 0.9;

std::string output_filename;
std::string input_filename;
std::string pgo_filename;
std::string mode = "none";

void parse_command_line(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--output") == 0) {
            i += 1;
            output_filename = std::string(argv[i]);
            continue;
        }
        if (strcmp(argv[i], "--disable-jump-table-reloc") == 0) {
            enableJumptableReloc = false;
            continue;
        }
        if (strcmp(argv[i], "--disable-function-pointer-reloc") == 0) {
            enableFuncPointerReloc = false;
            continue;
        }

        if (strcmp(argv[i], "--coverage-mode") == 0) {
            mode = std::string(argv[i+1]);
            i += 1;
            continue;
        }

        if (strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
            continue;
        }

        if (strcmp(argv[i], "--use-global-memory") == 0) {
            threadLocalMemory = false;
            continue;
        }

        if (strcmp(argv[i], "--use-thread-local-memory") == 0) {
            threadLocalMemory = true;
            continue;
        }

        if (strcmp(argv[i], "--insert-nops") == 0) {
            nops = atoi(argv[i+1]);
            i += 1;
            continue;
        }

        if (strcmp(argv[i], "--pgo-file") == 0) {
            pgo_filename = argv[i+1];
            LoopCloneOptimizer::readPGOFile(pgo_filename);
            i += 1;
            continue;
        }

        if (strcmp(argv[i], "--loop-clone-limit") == 0) {
            loop_clone_limit = atoi(argv[i+1]);
            i += 1;
            continue;
        }

        if (strcmp(argv[i], "--pgo-ratio") == 0) {
            pgo_ratio = strtod(argv[i+1], NULL);
            i += 1;
            continue;
        }

        if (argv[i][0] == '-') {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            exit(1);
        }
        input_filename = std::string(argv[i]);
    }
}

static bool skipFunction(BPatch_function * f) {
    // Skip functions not in .text
    PatchFunction * patchFunc = Dyninst::PatchAPI::convert(f);
    ParseAPI::Function* parseFunc = patchFunc->function();
    ParseAPI::SymtabCodeRegion* scr = static_cast<ParseAPI::SymtabCodeRegion*>(parseFunc->region());
    SymtabAPI::Region* r = scr->symRegion();
    return r->getRegionName() != ".text";
}

void InstrumentBlock(BPatch_function *f, BPatch_basicBlock* b) {
    Snippet::Ptr coverage = threadLocalMemory ?
        ThreadLocalMemCoverageSnippet::create(new ThreadLocalMemCoverageSnippet()) :
        GlobalMemCoverageSnippet::create(new GlobalMemCoverageSnippet());

    BPatch_point* bp = b->findEntryPoint();
    assert(bp != nullptr);

    Point* p = Dyninst::PatchAPI::convert(bp, BPatch_callBefore);
    assert(p != nullptr);
    p->pushBack(coverage);
}

int main(int argc, char** argv) {
    parse_command_line(argc, argv);
    bpatch.setRelocateJumpTable(enableJumptableReloc);
    bpatch.setRelocateFunctionPointer(enableFuncPointerReloc);
    binEdit = bpatch.openBinary(input_filename.c_str());
    image = binEdit->getImage();
    std::vector<BPatch_function*>* funcs = image->getProcedures();

    std::map<BPatch_function*, std::set<BPatch_basicBlock*> > instBlocksMap;

    for (auto f : *funcs) {
        if (skipFunction(f)) continue;
        f->setLayoutOrder((uint64_t)(f->getBaseAddr()));
        PatchFunction *pf = Dyninst::PatchAPI::convert(f);
        BPatch_flowGraph* cfg = f->getCFG();
        std::set<BPatch_basicBlock*> blocks;
        cfg->getAllBasicBlocks(blocks);
        if (verbose) {
            printf("Function %s at %lx\n", pf->name().c_str(), pf->addr());
        }
        CoverageLocationOpt clo(pf, mode, verbose);

        std::set<BPatch_basicBlock*> &instBlocks = instBlocksMap[f];
        for (auto b: blocks) {
            PatchBlock *pb = Dyninst::PatchAPI::convert(b);
            if (!clo.needInstrumentation(pb->start())) {
                if (verbose) {
                    printf("\tskip block [%lx, %lx)\n", pb->start(), pb->end());
                }
                continue;
            }
            if (verbose) {
                printf("\tinstrument block [%lx, %lx)\n", pb->start(), pb->end());
            }
            instBlocks.insert(b);
        }

        if (pgo_filename == "") {
            // Baseline instrumentation
            for (auto b : instBlocks) {
                InstrumentBlock(f, b);
            }
        }
    }
    if (pgo_filename != "") {
        LoopCloneOptimizer lco(instBlocksMap);
        lco.instrument();
    }
    binEdit->writeFile(output_filename.c_str());
}
