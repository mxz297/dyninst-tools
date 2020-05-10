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

BPatch_function* FindFunction(BPatch_image *image, const char* name) {
    std::vector<BPatch_function*> funcs;
    image->findFunction(name , funcs);
    if (funcs.size() != 1) return NULL;
    return funcs[0];
}

int main(int argc, char** argv) {
    binEdit = bpatch.openBinary(argv[1]);
    image = binEdit->getImage();
    std::vector<BPatch_function*>* funcs = image->getProcedures();
    BPatch_function* main_func = FindFunction(image, "__do_global_dtors_aux");

    binEdit->loadLibrary("libcg.so");

    BPatch_function* call_graph_func = FindFunction(image, "call_edge");
    BPatch_function* print_func = FindFunction(image, "print_call_graph");
    
    for (auto f : *funcs) {
        std::vector<BPatch_point*> points;
        f->getCallPoints(points);
        BPatch_constExpr from_addr( (uint64_t)(f->getBaseAddr()) );
        for (auto p : points) {
            vector<BPatch_snippet *> args;
            args.push_back(&from_addr);
            BPatch_function * callee = p->getCalledFunction();
            if (callee == NULL) {
                args.push_back(new BPatch_dynamicTargetExpr());
            } else {
                args.push_back(new BPatch_constExpr((uint64_t)(callee->getBaseAddr())));
            }
            BPatch_funcCallExpr call_edge(*call_graph_func, args);
            binEdit->insertSnippet(call_edge, *p, BPatch_callBefore, BPatch_firstSnippet);
        }
    }

    std::vector<BPatch_point*> points;
    main_func->getExitPoints(points);
    vector<BPatch_snippet*> args;
    BPatch_funcCallExpr print(*print_func, args);
    binEdit->insertSnippet(print, points, BPatch_callAfter, BPatch_firstSnippet);
    binEdit->writeFile(argv[2]);
}
