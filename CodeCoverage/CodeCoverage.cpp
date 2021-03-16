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

using namespace Dyninst;
using namespace PatchAPI;

BPatch bpatch;
BPatch_binaryEdit *binEdit;
BPatch_image* image;

bool instSharedLibs = false;
bool enableJumptableReloc = true;
bool enableFuncPointerReloc = true;

bool threadLocalMemory = true;

int padding_bytes = 0;
std::string output_filename;
std::string input_filename;
std::string mode = "none";
bool verbose = false;

class GlobalMemCoverageSnippet : public Dyninst::PatchAPI::Snippet {

public:
    explicit GlobalMemCoverageSnippet(Address addr): memLoc(addr) {}
    bool generate(Dyninst::PatchAPI::Point* pt, Dyninst::Buffer& buf) override {
        // Instruction template:
        // c6 05 37 e5 33 00 01    movb   $0x1,0x33e537(%rip)
        const int InstLength = 7;
        char code[InstLength];
        code[0] = 0xc6;
        code[1] = 0x05;
        code[6] = 0x1;
        int32_t disp = ((int64_t)memLoc) - (InstLength + buf.curAddr());
        *(int32_t*)(&code[2]) = disp;
        buf.copy(code, InstLength);
        if (verbose) {
            Dyninst::PatchAPI::PatchBlock* b = pt->block();
            printf("Block [%lx, %lx), bit allocated at %lx, relocated code at %lx, disp %x\n", b->start(), b->end(), memLoc, buf.curAddr(), disp);
        }
        return true;
    }

private:
    Address memLoc;
};

class ThreadLocalMemCoverageSnippet : public Dyninst::PatchAPI::Snippet {

public:
    explicit ThreadLocalMemCoverageSnippet(int o) : offset(o) {}
    bool generate(Dyninst::PatchAPI::Point* pt, Dyninst::Buffer& buf) override {
        // Instruction template:
        // 65 c6 04 25 87 01 00 00 01    movb   $0x1,%gs:0x187
        const int InstLength = 9;
        char code[InstLength];
        code[0] = 0x65;
        code[1] = 0xc6;
        code[2] = 0x04;
        code[3] = 0x25;
        code[8] = 0x1;
        *(int32_t*)(&code[4]) = offset;
        buf.copy(code, InstLength);
        if (verbose) {
            Dyninst::PatchAPI::PatchBlock* b = pt->block();
            printf("Block [%lx, %lx), bit offset at %x, relocated code at %lx\n", b->start(), b->end(), offset, buf.curAddr());
        }
        return true;
    }
private:
    int offset;
};

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
    Snippet::Ptr coverage;
    if (!threadLocalMemory) {
        Address memLoc = binEdit->allocateStaticMemoryRegion(1, "");
        coverage = GlobalMemCoverageSnippet::create(new GlobalMemCoverageSnippet(memLoc));
    } else {
        static int offset = 0;
        coverage = ThreadLocalMemCoverageSnippet::create(new ThreadLocalMemCoverageSnippet(offset));
        offset += 1;
    }

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

    for (auto f : *funcs) {
        if (skipFunction(f)) continue;
        f->setLayoutOrder((uint64_t)(f->getBaseAddr()));
        BPatch_flowGraph* cfg = f->getCFG();
        std::set<BPatch_basicBlock*> blocks;
        PatchFunction *pf = Dyninst::PatchAPI::convert(f);
        cfg->getAllBasicBlocks(blocks);
        if (verbose) {
            printf("Function %s at %lx\n", pf->name().c_str(), pf->addr());
        }
        CoverageLocationOpt clo(pf, mode, verbose);
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
            InstrumentBlock(f, b);
        }
    }
    binEdit->writeFile(output_filename.c_str());
}
