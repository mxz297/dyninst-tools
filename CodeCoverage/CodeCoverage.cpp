#include "BPatch.h"
#include "BPatch_function.h"
#include "BPatch_object.h"
#include "BPatch_flowGraph.h"
#include "BPatch_basicBlock.h"
#include "BPatch_point.h"

#include <cstring>


using namespace Dyninst;
using namespace PatchAPI;

BPatch bpatch;
BPatch_binaryEdit *binEdit;
BPatch_image* image;

bool instSharedLibs = false;
bool enableJumptableReloc = true;
bool enableFuncPointerReloc = true;
int padding_bytes = 0;
std::string output_filename;
std::string input_filename;

class CoverageSnippet : public Dyninst::PatchAPI::Snippet {

public:
    explicit CoverageSnippet(Address addr): memLoc(addr) {}
    bool generate(Dyninst::PatchAPI::Point* pt, Dyninst::Buffer& buf) override {
        // Instruction template:
        // c6 05 37 e5 33 00 01    movb   $0x1,0x33e537(%rip)
        const int InstLength = 7;
        char code[InstLength];
        code[0] = 0xc6;
        code[1] = 0x05;
        code[6] = 0x1;
        int32_t disp = ((int64_t)memLoc) + InstLength - buf.curAddr();
        *(int32_t*)(&code[2]) = disp;
        buf.copy(code, InstLength);
        return true;
    }

private:
    Address memLoc;
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
        if (argv[i][0] == '-') {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            exit(1);
        }
        input_filename = std::string(argv[i]);
    }
}

void InstrumentBlock(BPatch_function *f, BPatch_basicBlock* b) {
    Address memLoc = binEdit->allocateStaticMemoryRegion(1, "");
    Snippet::Ptr coverage = CoverageSnippet::create(new CoverageSnippet(memLoc));

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
        f->setLayoutOrder((uint64_t)(f->getBaseAddr()));
        BPatch_flowGraph* cfg = f->getCFG();
        std::set<BPatch_basicBlock*> blocks;
        cfg->getAllBasicBlocks(blocks);
        for (auto b: blocks) {
            InstrumentBlock(f, b);
        }
    }
    binEdit->writeFile(output_filename.c_str());
}
