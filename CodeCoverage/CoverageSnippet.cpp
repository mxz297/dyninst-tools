#include "CoverageSnippet.hpp"

#include "BPatch_binaryEdit.h"

extern int nops;
extern BPatch_binaryEdit *binEdit;

static int gsOffset = 0;

bool CoverageSnippet::generateNOPs(Dyninst::Buffer& buf) {
    char code[1];
    code[0] = 0x90;
    for (int i = 0; i < nops; ++i) {
        buf.copy(code, 1);
    }
    return true;
}

bool GlobalMemCoverageSnippet::generate(Dyninst::PatchAPI::Point* pt, Dyninst::Buffer& buf) {
    Dyninst::Address memLoc = binEdit->allocateStaticMemoryRegion(1, "");
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
    generateNOPs(buf);
    return true;
}

bool ThreadLocalMemCoverageSnippet::generate(Dyninst::PatchAPI::Point* pt, Dyninst::Buffer& buf) {
    int offset = gsOffset++;
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
    generateNOPs(buf);
    return true;
}