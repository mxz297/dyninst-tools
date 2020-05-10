#include <map>
#include <vector>
#include <cstdio>
#include <algorithm>
#include <stdlib.h>

std::map<uint64_t, std::map<uint64_t, uint64_t> > call_graph;

struct CallGraphEdge {
    uint64_t from;
    uint64_t to;
    uint64_t count;
    CallGraphEdge(uint64_t f, uint64_t t, uint64_t c):
        from(f), to(t), count(c)
    {}
    bool operator< (const CallGraphEdge& rhs) const {
        if (count != rhs.count)
            return count > rhs.count;
        if (from != rhs.from)
            return from < rhs.from;
        return to < rhs.to;
    }

};

extern "C" {

void call_edge(uint64_t from, uint64_t to) {
    call_graph[from][to] += 1;
}

void print_call_graph() {
    fprintf(stderr, "Print the call graph\n");
    char* filename = getenv("CALL_GRAPH_OUTPUT");
    FILE* outFile = NULL;
    if (filename != NULL) {
        outFile = fopen(filename, "w");
    }
    if (outFile == NULL) outFile = stdout;
    std::vector<CallGraphEdge> edges;
    for (auto it : call_graph) {
        uint64_t from = it.first;
        for (auto it2: it.second) {
            uint64_t to = it2.first;
            edges.push_back(CallGraphEdge(from, to, it2.second));
        }
    }
    std::sort(edges.begin(), edges.end());
    fprintf(outFile, "%u\n", edges.size());
    for (auto e : edges) {
        fprintf(outFile, "%lx %lx %lu\n", e.from, e.to, e.count);
    }
}

}
