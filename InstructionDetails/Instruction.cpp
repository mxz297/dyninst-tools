#include "Instruction.h"
#include "InstructionDecoder.h"
#include "Register.h"
#include "Expression.h"

using namespace std;
using namespace Dyninst;
using namespace Dyninst::InstructionAPI;

std::string convertRegSet(std::set<RegisterAST::Ptr> &regs) {
    vector<string> regStr;
    for (auto r : regs)
        regStr.push_back(r->format());
    string ret;
    for (auto s : regStr) ret += s + " ";
    return ret;
}

std::string convertExpressionSet(std::set<Expression::Ptr> &exprs) {
    vector<string> exprStr;
    for (auto e : exprs)
        exprStr.push_back(e->format());
    string ret;
    for (auto s : exprStr) ret += s + " ";
    return ret;
}

int main(int argc, char **argv) {
    unsigned char buf[] = {0xf2, 0x0f, 0x10, 0x05, 0x0f, 0x80, 0x09, 0x00};
    InstructionDecoder dec(buf, sizeof(buf), Arch_x86_64);

    Instruction insn = dec.decode();
    printf("%s\n", insn.format().c_str());
    
    printf("\tInstruction length: %u\n", insn.size());
    std::set<RegisterAST::Ptr> regsWritten;
    insn.getWriteSet(regsWritten);
    printf("\tRegister written: %s\n", convertRegSet(regsWritten).c_str()); 
    std::set<RegisterAST::Ptr> regsRead;
    insn.getReadSet(regsRead);
    printf("\tRegister read: %s\n", convertRegSet(regsRead).c_str()); 
    printf("\tReads memory: %d\n", insn.readsMemory());
    printf("\tWrites memory: %d\n", insn.writesMemory());
        
    std::set<Expression::Ptr> memAccessors;
    insn.getMemoryReadOperands(memAccessors);
    printf("\tgetMemoryReadOperands: %s\n", convertExpressionSet(memAccessors).c_str());

    memAccessors.clear();
    insn.getMemoryWriteOperands(memAccessors);
    printf("\tgetMemoryWriteOperands: %s\n", convertExpressionSet(memAccessors).c_str());

    string cft;
    if (insn.getControlFlowTarget()) cft = insn.getControlFlowTarget()->format();
    printf("\tgetControlFlowTarget: %s\n", cft.c_str());
    printf("\tallowsFallThrough: %d\n", insn.allowsFallThrough());
    printf("\tisValid: %d\n", insn.isValid());
    printf("\tisLegalInsn: %d\n", insn.isLegalInsn());
    printf("\tgetCategory: %d\n", insn.getCategory());
    
    vector<Operand> operands;
    insn.getOperands(operands);

    printf("\tOperands count %d\n", operands.size());
    for (auto op : operands) {
        printf("\t\top size %d\n", op.getValue()->size());
        printf("\t\top expression format %s\n", op.getValue()->format().c_str());

        regsRead.clear();
        op.getReadSet(regsRead);
        printf("\t\top register read: %s\n", convertRegSet(regsRead).c_str());

        regsWritten.clear();
        op.getWriteSet(regsWritten);
        printf("\t\top register written: %s\n", convertRegSet(regsWritten).c_str());
        printf("\t\top isRead: %d\n", op.isRead());
        printf("\t\top isWritten: %d\n", op.isWritten());
        printf("\t\top isImplicit: %d\n", op.isImplicit());
        printf("\t\top readsMemory: %d\n", op.readsMemory());
        printf("\t\top writesMemory: %d\n", op.writesMemory());
    }
}
