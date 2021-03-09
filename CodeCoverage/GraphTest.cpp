#include "SingleBlockGraph.hpp"
#include "MultiBlockGraph.hpp"
#include "CoverageLocationOpt.hpp"

using Dyninst::PatchAPI::PatchBlock;
using namespace GraphAnalysis;

int main(int argc, char** argv) {
    FILE* f = fopen(argv[1], "r");
    int n, e;
    fscanf(f, "%d %d", &n, &e);

    SingleBlockGraph::Ptr cfg = std::make_shared<SingleBlockGraph>();
    for (int i = 1; i <= n; ++i) {
        SBGNode::Ptr node = std::make_shared<SBGNode>((PatchBlock*)(i));
        cfg->addSBGNode(node);
    }

    for (int i = 1; i <= e; ++i) {
        int s, t;
        fscanf(f, "%d %d", &s, &t);
        SBGNode::Ptr source = cfg->lookupNode((PatchBlock*)(s));
        SBGNode::Ptr target = cfg->lookupNode((PatchBlock*)(t));
        source->addOutEdge(target);
        target->addInEdge(source);
    }
    cfg->addEntry(cfg->lookupNode((PatchBlock*)1));
    cfg->addExit(cfg->lookupNode((PatchBlock*)n));
    printf("CFG\n");
    cfg->Print(false);

    SingleBlockGraph::Ptr dominatorGraph = cfg->buildDominatorGraph();
    printf("Dominator Graph\n");
    dominatorGraph->Print(false);

    MultiBlockGraph::Ptr sbdg = std::make_shared<MultiBlockGraph>(cfg);
    printf("Superblock Dominator Graph\n");
    sbdg->Print(false);

    CoverageLocationOpt clo(cfg, sbdg, std::string("exact"));
    printf("Blocks to instrument for exact coverage\n");
    for (int i = 1; i <= n; ++i) {
        if (clo.needInstrumentation(i)) printf("\t <%d>\n", i);
    }
}