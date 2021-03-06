#include "Graph.hpp"
#include <assert.h>

namespace GraphAnalysis {

void Node::addInEdge(Node::Ptr n) {
    inEdges.emplace_back(n);
}

void Node::addOutEdge(Node::Ptr n) {
    outEdges.emplace_back(n);
}

Node::Ptr Node::eval() {
    if (ancestor == nullptr) {
        return label;
    }

    compress();
    if (ancestor->label->sdno() >= label->sdno()) {
        return label;
    } else {
        return ancestor->label;
    }
}

void Node::compress() {
    if (ancestor->ancestor == nullptr) {
        return;
    }

    ancestor->compress();
    if (ancestor->label->sdno() < label->sdno()) {
        label = ancestor->label;
    }
    assert(ancestor->ancestor->ancestor.get() != this);
    ancestor = ancestor->ancestor;
}

int Node::sdno() {
    return semiDom->dfs_no;
}

void Node::clearDominatorInfo(Node::Ptr in) {
    dfs_no = -1;
    size = 1;
    semiDom = label = in;
    ancestor = parent = child = immDom = nullptr;
    bucket.clear();
}

void Node::Print(bool realCode) {    
    PrintNodeData(realCode);
    fprintf(stderr, "\n\tout edges:");
    for (auto& n : outEdgeList()) {
        fprintf(stderr, " ");
        n->PrintNodeData(realCode);
    }
    fprintf(stderr, "\n\tin edges:");
    for (auto& n : inEdgeList()) {
        fprintf(stderr, " ");
        n->PrintNodeData(realCode);
    }
    fprintf(stderr, "\n");
}

void Node::PrintNodeData(bool) {
    fprintf(stderr, "Node<%p>", this);
}

void Graph::addNode(Node::Ptr n) {
    allNodes.emplace_back(n);
}

void Graph::addEntry(Node::Ptr n) {
    entries.emplace_back(n);
}

void Graph::addExit(Node::Ptr n) {
    exits.emplace_back(n);
}

void Graph::initializeDominatorInfo() {
    currentDepthNo = 0;
    naturalOrder.clear();
    reverseOrder.clear();
    for (auto& n : allNodes) {
        n->clearDominatorInfo(n);
    }
}

void Graph::dominatorTree(EdgeList& elist) {
    // Initialize states
    initializeDominatorInfo();

    for (auto& n: entries) {
        DFS(n, TraversalDirection::Natural);
    }

    dominatorComputation(elist, TraversalDirection::Reverse);
}

void Graph::postDominatorTree(EdgeList& elist) {
    // Initialize states
    initializeDominatorInfo();
    for (auto& n: exits) {
        DFS(n, TraversalDirection::Reverse);
    }

    dominatorComputation(elist, TraversalDirection::Natural);
}

void Graph::dominatorComputation(EdgeList& output, Graph::TraversalDirection dir) {
    for (size_t i = naturalOrder.size()-1; i > 0; i--) {
        Node::Ptr block = naturalOrder[i];
        Node::Ptr parent = block->parent;
        if (block->dfs_no == -1) {
            continue;
        }

        const Node::EdgeList& edgelist = (dir == TraversalDirection::Natural) ?
            block->outEdgeList() : block->inEdgeList();

        for (auto &s : edgelist) {
            Node::Ptr pred = s->eval();
            if (pred->sdno() < block->sdno()) {
                block->semiDom = pred->semiDom;
            }
        }

        block->semiDom->bucket.insert(block);
        if (parent == nullptr) continue;
        link(parent, block);

        while (!parent->bucket.empty()) {
            Node::Ptr u, v;
            v = *(parent->bucket.begin());
            parent->bucket.erase(parent->bucket.begin());
            u = v->eval();
            if (u->sdno() < v->sdno()) {
                v->immDom = u;
            } else {
                v->immDom = parent;
            }
        }
    }

    for (auto &block :  naturalOrder) {
        if (block->immDom != block->semiDom && block->immDom != nullptr) {
            block->immDom = block->immDom->immDom;
        }
        if (block->immDom != nullptr) {
            output.emplace_back(std::make_pair(block->immDom, block));
        }
    }
}

void Graph::SCC(std::vector< std::set<Node::Ptr> > &sccList){
    initializeDominatorInfo();
    for (auto& n: allNodes) {
        if (n->dfs_no != -1) continue;
        DFS(n, TraversalDirection::Natural);
    }

    std::vector<Node::Ptr> order = reverseOrder;
    initializeDominatorInfo();
    size_t curIndex = 0;
    for (auto it = order.rbegin(); it != order.rend(); ++it) {
        Node::Ptr n = *it;
        if (n->dfs_no != -1) continue;
        std::set<Node::Ptr> scc;
        DFS(n, TraversalDirection::Reverse);
        for ( ;curIndex < naturalOrder.size(); ++curIndex) {
            scc.insert(naturalOrder[curIndex]);
        }
        curIndex = naturalOrder.size();
        sccList.emplace_back(scc);
    }
}

void Graph::link(Node::Ptr v, Node::Ptr w) {
    Node::Ptr s = w;
    while (s->child != nullptr && w->label->sdno() < s->child->label->sdno()) {
        if (s->child->child != nullptr && s->size + s->child->child->size >= 2*s->child->size) {
            assert(s->ancestor != s->child);
            s->child->ancestor = s;
            s->child = s->child->child;
        } else {
            s->child->size = s->size;
            assert(s->child->ancestor != s);
            s->ancestor = s->child;
            s = s->child;
        }
    }

    s->label = w->label;
    v->size += w->size;
    if (v->size < 2 * w->size) {
        Node::Ptr tmp = s;
        s = v->child;
        v->child = tmp;
    }

    while (s != nullptr) {
        assert(v->ancestor != s);
        s->ancestor = v;
        s = s->child;
    }
}

void Graph::DFS(Node::Ptr v, Graph::TraversalDirection dir) {
    v->dfs_no = currentDepthNo++;
    naturalOrder.emplace_back(v);
    v->semiDom = v;
    const Node::EdgeList& edgelist = (dir == TraversalDirection::Natural) ?
        v->outEdgeList() : v->inEdgeList();
    for (const auto& succ : edgelist) {
        if (succ->dfs_no != -1) continue;
        succ->parent = v;
        DFS(succ, dir);
    }
    reverseOrder.emplace_back(v);
}

void Graph::Print(bool realCode) {
    fprintf(stderr, "Total number of nodes: %lu\n", allNodes.size());
    fprintf(stderr, "Entries\n");
    std::set<Node::Ptr> printed;
    for (auto &n : entries) {
        n->Print(realCode);
        printed.insert(n);
    }
    fprintf(stderr, "Exits\n");
    for (auto &n : exits) {
        n->Print(realCode);
        printed.insert(n);
    }
    fprintf(stderr, "Other nodes\n");
    for (auto &n : allNodes) {
        if (printed.find(n) != printed.end()) continue;
        n->Print(realCode);
    }
}

} // namespace GraphAnalysis