#include "CodeSource.h"
#include "CodeObject.h"
#include "CFG.h"
#include "Instruction.h"

using namespace std;
using namespace Dyninst;
using namespace Dyninst::ParseAPI;
using namespace Dyninst::InstructionAPI;

CodeRegion *dyninstReg = NULL;
CodeRegion *textReg = NULL;

bool IsTrampoline(unsigned char firstByte, Address entry, CodeObject *co) {
    if (firstByte == 0x62) return true; // illegal instruction for signal based trampoline
    if (firstByte != 0xe9) return false; // Not a branch, then not a tramopline
    Function * f = co->findFuncByEntry(textReg, entry);
    assert(f);
    for (auto e :f->entry()->targets()) {
        Block* b = e->trg();
        if (b->region() == dyninstReg) return true;
    }
    return false;
}

int main(int argc, char **argv) {
    if (argc < 2 || argc > 4) {
        fprintf(stderr, "Usage: %s <instr-binary> <original binary>\n Print statistics of binary rewriting\n", argv[0]);
        exit(-1);
    }

    SymtabCodeSource *sts_inst;
    CodeObject *co_inst;

    sts_inst = new SymtabCodeSource(argv[1]);    
    co_inst = new CodeObject(sts_inst);
    co_inst->parse();   

    // Find the .dyninstInst section
    vector<CodeRegion*> regs = sts_inst->regions();
    for (auto reg : regs) {
        SymtabAPI::Region *sym_reg = ((SymtabCodeRegion*)reg)->symRegion();
        string name = sym_reg->getRegionName();
        if (name == ".dyninstInst") {
            dyninstReg = reg;
        }
        if (name == ".text") {
            textReg = reg;
        }
    }

    if (dyninstReg == NULL) {
        fprintf(stderr, "Cannot find .dyninstInst section in %s\n", argv[1]);
        exit(1);
    }
    SymtabCodeSource *sts_orig;
    CodeObject *co_orig;

    sts_orig = new SymtabCodeSource(argv[2]);    
    co_orig = new CodeObject(sts_orig);
    co_orig->parse();   

    for (auto reg : sts_orig->regions()) {
        SymtabAPI::Region *sym_reg = ((SymtabCodeRegion*)reg)->symRegion();
        string name = sym_reg->getRegionName();
        if (name == ".text")
            co_orig->parseGaps(reg);
    }

    set<Address> func_entry;
    int total_inst = 0;
    int total_single = 0;
    for (auto func : co_orig->funcs()) {
        Address entry = func->addr();
        unsigned char* buffer = (unsigned char *) sts_inst->getPtrToInstruction(entry);
        if (buffer == NULL) {
            fprintf(stderr, "Cannot get buffer for %lx\n", entry);
            continue;
        }
        if (IsTrampoline(buffer[0], entry, co_inst)) {
            ++total_inst;
            func_entry.insert(func->addr());
        }
    }
    for (auto a : func_entry)
        printf("%lx\n", a);
}
