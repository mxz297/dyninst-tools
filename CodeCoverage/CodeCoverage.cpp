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
#include <iostream>
#include <fstream>


#include "CoverageLocationOpt.hpp"
#include "CoverageSnippet.hpp"
#include "LoopCloneOptimizer.hpp"

using namespace Dyninst;
using namespace PatchAPI;

BPatch bpatch;
BPatch_binaryEdit *binEdit;
BPatch_image* image;

bool instSharedLibs = false;
bool enableJumptableReloc = true;
bool enableFuncPointerReloc = true;
bool threadLocalMemory = true;
bool verbose = false;
bool emptyInst = false;
bool enableProfile = false;

int nops = 0;
int loop_clone_limit = 5;

double pgo_ratio = 0.9;

extern int gsOffset;

std::string output_filename;
std::string input_filename;
std::string pgo_address_filename;
std::string pgo_call_filename;
std::string mode = "none";

std::vector< std::pair<Address, Address> > callpairs;

void readPGOCallFile(std::string &filename) {
    std::ifstream infile(filename, std::fstream::in);
    uint64_t caller, callee;
    double metric;
    while (infile >> std::hex >> caller >> callee >> metric) {
        callpairs.emplace_back(std::make_pair(caller, callee));
    }
}

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

        if (strcmp(argv[i], "--insert-nops") == 0) {
            nops = atoi(argv[i+1]);
            i += 1;
            continue;
        }

        if (strcmp(argv[i], "--pgo-address-file") == 0) {
            pgo_address_filename = argv[i+1];
            LoopCloneOptimizer::readPGOFile(pgo_address_filename);
            i += 1;
            continue;
        }

        if (strcmp(argv[i], "--pgo-call-file") == 0) {
            pgo_call_filename = argv[i+1];
            readPGOCallFile(pgo_call_filename);
            i += 1;
            continue;
        }


        if (strcmp(argv[i], "--loop-clone-limit") == 0) {
            loop_clone_limit = atoi(argv[i+1]);
            i += 1;
            continue;
        }

        if (strcmp(argv[i], "--pgo-ratio") == 0) {
            pgo_ratio = strtod(argv[i+1], NULL);
            i += 1;
            continue;
        }

        if (strcmp(argv[i], "--empty-inst") == 0) {
            emptyInst = true;
            continue;
        }

        if (strcmp(argv[i], "--enable-profile") == 0) {
            enableProfile = true;
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
    if (parseFunc->retstatus() == ParseAPI::NORETURN) return true;
    ParseAPI::SymtabCodeRegion* scr = static_cast<ParseAPI::SymtabCodeRegion*>(parseFunc->region());
    SymtabAPI::Region* r = scr->symRegion();
    return r->getRegionName() != ".text";
}

void InstrumentBlock(BPatch_function *f, BPatch_basicBlock* b) {
    Snippet::Ptr coverage = threadLocalMemory ?
        ThreadLocalMemCoverageSnippet::create(new ThreadLocalMemCoverageSnippet()) :
        GlobalMemCoverageSnippet::create(new GlobalMemCoverageSnippet());

    BPatch_point* bp = b->findEntryPoint();
    assert(bp != nullptr);

    Point* p = Dyninst::PatchAPI::convert(bp, BPatch_callBefore);
    assert(p != nullptr);
    p->pushBack(coverage);
}

void determineInstrumentationOrder(std::vector<BPatch_function*> &funcs) {
    std::map<Address, std::vector<Address>* > addr_maps;
    for (auto& pair : callpairs) {
        Address &from = pair.first;
        Address &to = pair.second;
        if (from == to) continue;
        auto from_it = addr_maps.find(from);
        if (from_it == addr_maps.end()) {
            std::vector<Address>* from_vec = new std::vector<Address>;
            from_vec->push_back(from);
            from_it = addr_maps.insert(make_pair(from, from_vec)).first;
        }
        auto to_it = addr_maps.find(to);
        if (to_it == addr_maps.end()) {
            from_it->second->push_back(to);
        } else if (to_it->second != from_it->second) {
            std::vector<Address>* to_vec = to_it->second;
            from_it->second->insert(from_it->second->end(), to_vec->begin(), to_vec->end());
            for (auto addr: *to_vec) {
                addr_maps[addr] = from_it->second;
            }
            delete to_vec;
        }
    }
    std::vector<Address> order;
    set< std::vector<Address>* > processed;
    for (auto it : addr_maps) {
        if (processed.find(it.second) != processed.end()) continue;
        processed.insert(it.second);
        order.insert(order.end(), it.second->begin(), it.second->end());
    }
    std::map<Address, int> funcOrder;
    int i = 0;
    for (auto addr : order) {
        funcOrder[addr] = i++;
    }
    sort(funcs.begin(), funcs.end(),
        [&funcOrder] (BPatch_function* a, BPatch_function *b) {
            Address addra = (Address)(a->getBaseAddr());
            auto ita = funcOrder.find(addra);
            int orderA;
            if (ita == funcOrder.end()) orderA = 100000000; else orderA = ita->second;

            Address addrb = (Address)(b->getBaseAddr());
            auto itb = funcOrder.find(addrb);
            int orderB;
            if (itb == funcOrder.end()) orderB = 100000000; else orderB = itb->second;

            if (orderA != orderB) return orderA < orderB;
            return addra < addrb;
        }
    );
}

int main(int argc, char** argv) {
    parse_command_line(argc, argv);
    bpatch.setRelocateJumpTable(enableJumptableReloc);
    bpatch.setRelocateFunctionPointer(enableFuncPointerReloc);
    bpatch.setWriteAdressMappingForProfile(enableProfile);
    binEdit = bpatch.openBinary(input_filename.c_str());
    image = binEdit->getImage();
    std::vector<BPatch_function*>* origFuncs = image->getProcedures();
    std::vector<BPatch_function*> funcs = *origFuncs;
    determineInstrumentationOrder(funcs);

    std::map<BPatch_function*, std::set<BPatch_basicBlock*> > instBlocksMap;
    int skippedFunction = 0;
    std::map<int, std::set<uint64_t> > functionDist;
    std::set<Address> instrumented;
    for (auto f : funcs) {
        if (skipFunction(f)) {            
            skippedFunction += 1;
            continue;
        }
        f->setLayoutOrder((uint64_t)(f->getBaseAddr()));
        PatchFunction *pf = Dyninst::PatchAPI::convert(f);
        BPatch_flowGraph* cfg = f->getCFG();
        std::set<BPatch_basicBlock*> blocks;
        cfg->getAllBasicBlocks(blocks);
        if (verbose) {
            printf("Function %s at %lx\n", pf->name().c_str(), pf->addr());
        }
        CoverageLocationOpt clo(pf, mode, verbose);
        
        std::set<BPatch_basicBlock*> &instBlocks = instBlocksMap[f];
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
            instBlocks.insert(b);
            instrumented.insert(pb->start());
        }
        functionDist[instBlocks.size()].insert(pf->addr());

        if (pgo_address_filename == "") {
            // Baseline instrumentation
            for (auto b : instBlocks) {
                InstrumentBlock(f, b);
            }
        }
    }
    if (pgo_address_filename != "") {
        LoopCloneOptimizer lco(instBlocksMap, funcs);
        lco.instrument();
    }
    binEdit->writeFile(output_filename.c_str());
    printf("Finishing instrumentation %d functions, skipped %d functions\n", funcs.size() - skippedFunction, skippedFunction);
    printf("Require %d bytes memory in instrumentation region\n", gsOffset);

    int sum = 0;
    for (auto &rit : functionDist) {
        printf("%d %d %lx\n", rit.first, rit.second.size(), *(rit.second.begin()));
        sum += rit.first * rit.second.size();

    }
    printf("sum %d\n", sum);
    printf("Unique block address instrumented %d\n", instrumented.size());

}
