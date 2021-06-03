#include "BPatch.h"
#include "BPatch_function.h"
#include "BPatch_object.h"
#include "BPatch_flowGraph.h"
#include "BPatch_basicBlock.h"
#include "BPatch_point.h"

#include "PatchCFG.h"
#include "PatchObject.h"
#include "PatchModifier.h"

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
std::string pgo_inline_filename;
std::string mode = "none";
std::string coverage_file;

std::vector< std::pair<Address, Address> > callpairs;
std::vector< std::pair<Address, Address> > callsites;

void readPGOCallFile(std::string &filename) {
    std::ifstream infile(filename, std::fstream::in);
    uint64_t caller, callee;
    double metric;
    while (infile >> std::hex >> caller >> callee >> metric) {
        callpairs.emplace_back(std::make_pair(caller, callee));
    }
}

void readPGOInlineFile(std::string &filename) {
    std::ifstream infile(filename, std::fstream::in);
    uint64_t callsite, callee;
    double metric;
    while (infile >> std::hex >> callsite >> callee >> metric) {
        callsites.emplace_back(std::make_pair(callsite, callee));
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

        if (strcmp(argv[i], "--pgo-inline-file") == 0) {
            pgo_inline_filename = argv[i+1];
            readPGOInlineFile(pgo_inline_filename);
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

        if (strcmp(argv[i], "--print-coverage") == 0) {
            coverage_file = argv[i+1];
            i += 1;
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

void InstrumentBlock(PatchFunction *f, PatchBlock* b) {
    Snippet::Ptr coverage = threadLocalMemory ?
        ThreadLocalMemCoverageSnippet::create(new ThreadLocalMemCoverageSnippet(b->start())) :
        GlobalMemCoverageSnippet::create(new GlobalMemCoverageSnippet(b->start()));

    PatchMgr::Ptr mgr = f->obj()->mgr();
    Point* p = mgr->findPoint(Location::BlockInstance(f, b, true), Point::BlockEntry, true);
    assert(p != nullptr);
    p->pushBack(coverage);
}

void determineAnalysisOrder(std::vector<PatchFunction*> &funcs) {
    std::map<Address, int> funcsBlockCount;
    for (auto f: funcs) {
        int count = 0;
        for (auto b : f->blocks()) {
            count += 1;
        }
        funcsBlockCount[f->addr()] = count;
    }

    sort(funcs.begin(), funcs.end(),
        [&funcsBlockCount] (PatchFunction* a, PatchFunction *b) {
            Address addra = a->addr();
            Address addrb = b->addr();
            if (funcsBlockCount[addra] == funcsBlockCount[addrb]) {
                return addra < addrb;
            } else {
                return funcsBlockCount[addra] > funcsBlockCount[addrb];
            }
        }
    );

}

void determineInstrumentationOrder(std::vector<PatchFunction*> &funcs) {
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
        [&funcOrder] (PatchFunction* a, PatchFunction *b) {
            Address addra = a->addr();
            auto ita = funcOrder.find(addra);
            int orderA;
            if (ita == funcOrder.end()) orderA = 100000000; else orderA = ita->second;

            Address addrb = b->addr();
            auto itb = funcOrder.find(addrb);
            int orderB;
            if (itb == funcOrder.end()) orderB = 100000000; else orderB = itb->second;

            if (orderA != orderB) return orderA < orderB;
            return addra < addrb;
        }
    );
}

void performInlining(std::vector<PatchFunction*>& funcs) {
    std::map<Address, std::pair<PatchFunction*, PatchBlock*> > callSiteMap;
    PatchObject* obj = nullptr;
    for (auto f: funcs) {
        obj = f->obj();
        for (auto b : f->callBlocks()) {
            callSiteMap[b->end()] = std::make_pair(f, b);
        }
    }
    PatchModifier::beginInlineSet(obj);
    for (auto & callsite : callsites) {
        auto cit = callSiteMap.find(callsite.first);
        if (cit == callSiteMap.end()) continue;
        PatchFunction* caller = cit->second.first;
        PatchBlock* callBlock = cit->second.second;
        bool indirect = false;
        for (auto e : callBlock->targets()) {
            if (e->sinkEdge()) {
                indirect = true;
            }
        }
        if (indirect) continue;
        Address calleeAddress = callsite.second;
        if (PatchModifier::inlineCall(caller, callBlock, calleeAddress)) {
            printf("Inline callsite %lx, callee %lx\n", callsite.first, calleeAddress);
        }
    }
    PatchModifier::endInlineSet();
}

int main(int argc, char** argv) {
    parse_command_line(argc, argv);
    bpatch.setRelocateJumpTable(enableJumptableReloc);
    bpatch.setRelocateFunctionPointer(enableFuncPointerReloc);
    bpatch.setWriteAdressMappingForProfile(enableProfile);
    binEdit = bpatch.openBinary(input_filename.c_str());
    image = binEdit->getImage();
    std::vector<BPatch_function*>* origFuncs = image->getProcedures();
    std::vector<PatchFunction*> funcs;

    for (auto f : *origFuncs) {
        if (skipFunction(f)) {
            continue;
        }
        f->setLayoutOrder((uint64_t)(f->getBaseAddr()));
        // BPatch_flowGraph cannot be constructed in parallel;
        f->getCFG();
        PatchFunction *pf = Dyninst::PatchAPI::convert(f);
        funcs.emplace_back(pf);
    }

    performInlining(funcs);
    determineAnalysisOrder(funcs);

    tbb::concurrent_hash_map<PatchFunction*, std::set<PatchBlock*> > concurInstBlocksMap;
    size_t totalFunc = funcs.size();
    #pragma omp parallel for schedule(dynamic)
    for (size_t i = 0; i < totalFunc; ++i) {
        auto pf = funcs[i];
        if (verbose) {
            printf("Function %s at %lx\n", pf->name().c_str(), pf->addr());
        }
        CoverageLocationOpt clo(pf, mode, verbose);

        tbb::concurrent_hash_map<PatchFunction*, std::set<PatchBlock*> >::accessor a;
        assert(concurInstBlocksMap.insert(a, std::make_pair(pf, std::set<PatchBlock*>())));
        std::set<PatchBlock*> &instBlocks = a->second;
        for (auto b: pf->blocks()) {
            if (!clo.needInstrumentation(b->start())) {
                if (verbose) {
                    printf("\tskip block [%lx, %lx)\n", b->start(), b->end());
                }
                continue;
            }
            if (verbose) {
                printf("\tinstrument block [%lx, %lx)\n", b->start(), b->end());
            }
            instBlocks.insert(b);
        }
    }

    determineInstrumentationOrder(funcs);
    std::map<PatchFunction*, std::set<PatchBlock*> > instBlocksMap;
    for (auto pf : funcs) {
        tbb::concurrent_hash_map<PatchFunction*, std::set<PatchBlock*> >::accessor a;
        assert(concurInstBlocksMap.find(a, pf));
        instBlocksMap[pf] = a->second;
    }

    if (pgo_address_filename != "") {
        LoopCloneOptimizer lco(instBlocksMap, funcs);
        lco.instrument();
    } else {
        for (auto pf : funcs) {
            pf->markModified();
            for (auto b : instBlocksMap[pf]) {
                InstrumentBlock(pf, b);
            }
        }
    }

    binEdit->writeFile(output_filename.c_str());
    printf("Require %d bytes memory in instrumentation region\n", ThreadLocalMemCoverageSnippet::gsOffset);
    ThreadLocalMemCoverageSnippet::printCoverage(coverage_file);
    return 0;
}
