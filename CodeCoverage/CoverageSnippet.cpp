#include "CoverageSnippet.hpp"

#include "BPatch_binaryEdit.h"

using Dyninst::Address;

extern int nops;
extern BPatch_binaryEdit *binEdit;
extern bool emptyInst;

void CoverageSnippet::print() {
    printf("CoverageSnippet");
}

bool CoverageSnippet::generateNOPs(Dyninst::Buffer& buf) {
    char code[1];
    code[0] = 0x90;
    for (int i = 0; i < nops; ++i) {
        buf.copy(code, 1);
    }
    return true;
}

std::map<Address, Address> GlobalMemCoverageSnippet::locMap;

GlobalMemCoverageSnippet::GlobalMemCoverageSnippet(Address blockAddr) {
    auto it = locMap.find(blockAddr);    
    if (it == locMap.end()) {
        memLoc = binEdit->allocateStaticMemoryRegion(1, "");
        locMap.emplace(blockAddr, memLoc);
    } else {
        memLoc = it->second;
    }
}

bool GlobalMemCoverageSnippet::generate(Dyninst::PatchAPI::Point* pt, Dyninst::Buffer& buf) {
    // Instruction template:
    // c6 05 37 e5 33 00 01    movb   $0x1,0x33e537(%rip)
    if (emptyInst) return true;
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

void GlobalMemCoverageSnippet::print() {
    printf("GMSnippet<%lx>", memLoc);
}

int ThreadLocalMemCoverageSnippet::gsOffset = 0;
std::map<Address, int> ThreadLocalMemCoverageSnippet::locMap;

ThreadLocalMemCoverageSnippet::ThreadLocalMemCoverageSnippet(Address blockAddr) {
    auto it = locMap.find(blockAddr);
    if (it == locMap.end()) {
        offset = gsOffset++;
        locMap.emplace(blockAddr, offset);        
    } else {
        offset = it->second;
    }
}

bool ThreadLocalMemCoverageSnippet::generate(Dyninst::PatchAPI::Point* pt, Dyninst::Buffer& buf) {
    if (emptyInst) {
        unsigned char code[9] = {0x66, 0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00};        
        buf.copy(code, 9);
        return true;
    }
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

void ThreadLocalMemCoverageSnippet::print() {
    printf("TLMSnippet<%x>", offset);
}

void ThreadLocalMemCoverageSnippet::printCoverage(std::string& filename) {
    if (filename == "") return;
    FILE* f = fopen(filename.c_str(), "w");
    if (f == nullptr) return;
    fprintf(f, "%d\n", gsOffset);
    for (auto &it : locMap) {
        //fprintf(f, "%lx %d\n", it.first, it.second);
        fprintf(f, "%lx\n", it.first);
    }
    fclose(f);
}