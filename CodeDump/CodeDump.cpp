#include "CodeSource.h"
#include "CodeObject.h"
#include "CFG.h"
#include "Instruction.h"

using namespace std;
using namespace Dyninst;
using namespace Dyninst::ParseAPI;
using namespace Dyninst::InstructionAPI;

int main(int argc, char **argv) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: %s <binary>\nDump the discovered code from the input binary\n", argv[0]);
        exit(-1);
    }

    SymtabCodeSource *sts;
    CodeObject *co;

    sts = new SymtabCodeSource(argv[1]);    
    co = new CodeObject(sts);
    co->parse();   
    int count = 0;
    // Iterate all functions
    for (auto f : co->funcs()) {
        printf("Function at %lx with name %s\n",f->addr(), f->name().c_str());
        // Iterate all blocks in a function
        for (auto b : f->blocks()) {
            // Get all instrucitons in a block
            Block::Insns insns;
            b->getInsns(insns);
            for (auto insn_iter : insns) {
                printf("%lx : %s\n", insn_iter.first, insn_iter.second.format().c_str());
            }
        }
    }
}
