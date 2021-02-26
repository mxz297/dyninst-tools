#include "BPatch.h"
#include "BPatch_basicBlock.h"
#include "BPatch_edge.h"
#include "BPatch_flowGraph.h"
#include "BPatch_function.h"
#include "BPatch_object.h"
#include "BPatch_point.h"

#include <cstring>


using namespace Dyninst;

BPatch bpatch;
BPatch_binaryEdit *binEdit;
BPatch_image* image;

bool instSharedLibs = false;
bool enableJumptableReloc = true;
bool enableFuncPointerReloc = true;
int padding_bytes = 0;
std::string output_filename;
std::string input_filename;

void parse_command_line(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--libs") == 0) {
            instSharedLibs = true;
            continue;
        }
        if (strcmp(argv[i], "--padding") == 0) {
            i += 1;
            padding_bytes = atoi(argv[i]);
            continue;
        }
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

int main(int argc, char** argv) {
    parse_command_line(argc, argv);
    bpatch.setRelocateJumpTable(enableJumptableReloc);
    bpatch.setRelocateFunctionPointer(enableFuncPointerReloc);
    binEdit = bpatch.openBinary(input_filename.c_str(), instSharedLibs);
    image = binEdit->getImage();
    std::vector<BPatch_function*>* funcs = image->getProcedures();

    for (auto f : *funcs) {
        f->setLayoutOrder((uint64_t)(f->getBaseAddr()));
        BPatch_flowGraph* cfg = f->getCFG();
        std::set<BPatch_basicBlock*> blocks;
        cfg->getAllBasicBlocks(blocks);
        for (auto b : blocks) {
            BPatch_point * p = b->findEntryPoint();
            BPatch_paddingSnippet padding(padding_bytes);
            binEdit->insertSnippet(padding, *p, BPatch_callBefore, BPatch_firstSnippet);
        }
    }
    binEdit->writeFile(output_filename.c_str());
}
