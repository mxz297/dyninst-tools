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
        fprintf(stderr, "Usage: %s <binary>\nCheck CFG consistency generaetd by ParseAPI\n", argv[0]);
        exit(-1);
    }

    SymtabCodeSource *sts;
    CodeObject *co;

    sts = new SymtabCodeSource(argv[1]);    
    co = new CodeObject(sts);
    co->parse();   

    // Find the .text section
    vector<CodeRegion*> regs = sts->regions();
    CodeRegion *textReg = NULL;
    for (auto reg : regs) {
        SymtabAPI::Region *sym_reg = ((SymtabCodeRegion*)reg)->symRegion();
        string name = sym_reg->getRegionName();
        if (name == ".text") {
            textReg = reg;
            break;
        }
    }
    co->parseGaps(textReg, IdiomMatching);
    // Check CFG consistency 
    int count = 0;
    for (auto f : co->funcs()) {
        // Look up function by entry address
        Function * funcByEntry = co->findFuncByEntry(textReg, f->addr());
        if (funcByEntry != f) {
            count ++;
            fprintf(stderr, "Function %s at %lx (%p) not found. findFuncByEntry %p\n", f->name().c_str(), f->addr(), f, funcByEntry);
            if (funcByEntry) {
                fprintf(stderr, "\t found %s at %lx\n", funcByEntry->name().c_str(), funcByEntry->addr());
            }
        }
        for (auto b: f->blocks()) {
            Address midAddr = (b->start() + b->end()) / 2;
            set<Function*> funcs;
            co->findFuncs(textReg, midAddr, funcs);
            bool find = false;
            for (auto found_func : funcs) {
                if (found_func == f) {
                    find = true;
                    break;
                }
            }
            if (!find) {
                ++count;
                fprintf(stderr, "Address %lx in function %s at %lx not found.\n", midAddr, f->name().c_str(), f->addr());
            }
        }

        for (auto b : f->blocks()) {            
            // There should be a unique ParseAPI::Block for an address.
            // The block we got through iteration should be the same as
            // the one we found by looking up
            Block* findByEntry = co->findBlockByEntry(textReg, b->start());
            if (findByEntry != b) {
                count ++;
                fprintf(stderr, "Block [%lx, %lx) are not found. findBlockByEntry %p\n", b->start(), b->end(), findByEntry);
                if (findByEntry != NULL)
                    fprintf(stderr, "\tfindBlockByEntry returns a block [%lx,%lx)\n", findByEntry->start(), findByEntry->end());
            }

            // Looking up address using the mid point should
            // return a list of blocks containing the curernt block
            set<Block*> blocks;
            Address midAddr = (b->start() + b->end()) / 2;
            co->findBlocks(textReg, midAddr, blocks);
            bool find = false;
            for (auto found_block : blocks) {
                if (found_block == b) {
                    find = true;
                    break;
                }
            }
            if (!find) {
                count ++;
                fprintf(stderr, "Block [%lx, %lx), midAddr %lx not found, findBlocks does not find this block\n", b->start(), b->end(), midAddr);
                fprintf(stderr, "\t findBlocks return %lu blocks\n", blocks.size());
                for (auto bbit = blocks.begin(); bbit != blocks.end(); ++bbit) {
                    fprintf(stderr, "\t\t[%lx, %lx)\n", (*bbit)->start(), (*bbit)->end());
                }
            }

            // Check targets edges:
            // the target edge should be in the source edge list
            // of the target block
            for (auto e : b->targets()) {
                // Ignore sink edges
                if (e->sinkEdge()) continue;
                bool find = false;
                Block* targ = e->trg();
                for (auto se : targ->sources())
                    if (se == e) {
                        find = true;
                        break;
                    }
                if (!find) {
                    count ++;
                    fprintf(stderr, "Target edge from block [%lx, %lx) to block [%lx, %lx) is not found in source edge list\n", b->start(), b->end(), targ->start(), targ->end());
                }
            }

            // Check sources edges:
            // the source edge should be in the target edge list
            // of the source block
            for (auto e : b->sources()) {
                bool find = false;
                Block* source = e->src();
                for (auto te : source->targets())
                    if (te == e) {
                        find = true;
                        break;
                    }
                if (!find) {
                    count ++;
                    fprintf(stderr, "Source edge from block [%lx, %lx) to block [%lx, %lx) is not found in target edge list\n", source->start(), source->end(), b->start(), b->end());
                }
            }
        }
    }
    fprintf(stderr, "Total inconsistency %d\n", count);
}
