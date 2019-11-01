#include "CodeSource.h"
#include "CodeObject.h"
#include "CFG.h"
#include "Instruction.h"
#include "Operand.h"
#include "Visitor.h"
using namespace std;
using namespace Dyninst;
using namespace Dyninst::ParseAPI;
using namespace Dyninst::InstructionAPI;

set<int> offsets;

class RedZoneAccessVisitor: public Visitor {

    public:
    int disp;
    bool plusOp;
    bool findSP;
    bool onlySP;

    RedZoneAccessVisitor(): 
        disp(0), plusOp(false), findSP(false), onlySP(true)
    {}

    virtual void visit(BinaryFunction* b) {
        if (b->isAdd()) plusOp = true;
    }

    virtual void visit(Immediate* i) {
        const Result& r = i->eval();
        disp = r.convert<int>();
    }
    
    virtual void visit(RegisterAST* r) {
        if (r->getID() == x86_64::rsp) findSP = true; else onlySP = false;
    }
    virtual void visit(Dereference* d) {}

    bool isRedZoneAccess() {
        return plusOp && findSP && (disp < 0) && onlySP;
    }
};

bool ContainRedZoneAccesses(std::set<Expression::Ptr> &accessors) {
    for (auto e : accessors) {
        RedZoneAccessVisitor v;
        e->apply(&v);
        if (v.isRedZoneAccess()) {
            offsets.insert(v.disp);
            return true;
        }
    }
    return false;
}

int main(int argc, char **argv) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: %s <binary>\nFind red zone memory accesses\n", argv[0]);
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
        // Iterate all blocks in a function
        for (auto b : f->blocks()) {
            // Get all instrucitons in a block
            Block::Insns insns;
            b->getInsns(insns);

            for (auto insn_iter : insns) {
                bool redZoneAccess = false;
                Instruction i = insn_iter.second;
                // Skip instructions that do not access memory
                if (!i.writesMemory() && !i.readsMemory()) continue;
                if (i.getOperation().getID() == e_push) continue;

                std::set<Expression::Ptr> accessors;
                i.getMemoryReadOperands(accessors);
                if (ContainRedZoneAccesses(accessors)) {
                    redZoneAccess = true;
                } else {
                    accessors.clear();
                    i.getMemoryWriteOperands(accessors);
                    if (ContainRedZoneAccesses(accessors)) {
                        redZoneAccess = true;
                    } 
                }
                if (redZoneAccess) {
                    printf("%lx : %s\n", insn_iter.first, insn_iter.second.format().c_str());
                }
            }
        }
    }
    for (auto o : offsets) printf("%d\n", o);
}
