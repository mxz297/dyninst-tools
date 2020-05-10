#include "BPatch.h"
#include "BPatch_basicBlock.h"
#include "BPatch_edge.h"
#include "BPatch_flowGraph.h"
#include "BPatch_function.h"
#include "BPatch_object.h"
#include "BPatch_point.h"

using namespace Dyninst;

BPatch bpatch;
BPatch_binaryEdit *binEdit;
BPatch_image* image;
InstSpec is;

std::map<uint64_t, int> funcOrder;

void LoadCallGraph(const char* filename) {
    FILE* f = fopen(filename, "r");
    int totalEdges;
    fscanf(f, "%d", &totalEdges);
    std::map<uint64_t, std::vector<uint64_t>* > addr_maps; 

    for (int i = 0; i < totalEdges; ++i) {
        uint64_t from, to, count;
        fscanf(f, "%lx %lx %lu", &from, &to, &count);
        if (from == to) continue;
        auto from_it = addr_maps.find(from);
        if (from_it == addr_maps.end()) {
            std::vector<uint64_t>* from_vec = new std::vector<uint64_t>;
            from_vec->push_back(from);
            from_it = addr_maps.insert(make_pair(from, from_vec)).first;
        }
        auto to_it = addr_maps.find(to);
        if (to_it == addr_maps.end()) {
            from_it->second->push_back(to);
        } else if (to_it->second != from_it->second) {
            std::vector<uint64_t>* to_vec = to_it->second;
            from_it->second->insert(from_it->second->end(), to_vec->begin(), to_vec->end());
            for (auto addr: *to_vec) {
                addr_maps[addr] = from_it->second;
            }
            delete to_vec;
        }
    }
    std::vector<uint64_t> order;
    set< std::vector<uint64_t>* > processed;
    for (auto it : addr_maps) {
        if (processed.find(it.second) != processed.end()) continue;
        processed.insert(it.second);
        order.insert(order.end(), it.second->begin(), it.second->end());
    }
    int i = 0;
    for (auto addr : order) {
        printf("%lx\n", addr);
        funcOrder[addr] = i++;
    }
}

int main(int argc, char** argv) {
    bpatch.setRelocateJumpTable(true);
    bpatch.setRelocateFunctionPointer(true);
    is.trampGuard = false;
    is.redZone = false;
    LoadCallGraph(argv[3]);
    binEdit = bpatch.openBinary(argv[1]);
    image = binEdit->getImage();
    std::vector<BPatch_function*>* funcs = image->getProcedures();
    BPatch_nullExpr nopSnippet;
    for (auto f : *funcs) {
        if (f->getName() == "_fini") continue;
        if (f->getName() == "_init") continue;
        uint64_t addr = (uint64_t) (f->getBaseAddr());
        int order = 10000000;
        if (funcOrder.find(addr) != funcOrder.end()) order = funcOrder[addr];
        f->setLayoutOrder(order);
        vector<BPatch_point*> points;
        f->getEntryPoints(points);
        binEdit->insertSnippet(nopSnippet, points, BPatch_callBefore, BPatch_lastSnippet, &is);

    }
    binEdit->writeFile(argv[2]);
}
