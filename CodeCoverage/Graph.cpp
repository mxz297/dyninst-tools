#include "Graph.hpp"

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
    ancestor = ancestor->ancestor;
}

int Node::sdno() {
    return semiDom->dfs_no;
}

void Node::clearDominatorInfo() {
    dfs_no = -1;
    size = 1;
    semiDom.reset();
    immDom.reset();
    label.reset();
    ancestor.reset();
    parent.reset();
    child.reset();
    bucket.clear();
}

void Graph::addNode(Node::Ptr n) {
    allNodes.emplace_back(n);
}

void Graph::addEntry(Node::Ptr n) {
    entries.emplace_back(n);
}

void Graph::addExit(Node::Ptr n) {
    entries.emplace_back(n);
}

void Graph::initializeDominatorInfo() {
    currentDepthNo = 0;
    sorted_blocks.clear();
    for (auto& n : allNodes) {
        n->clearDominatorInfo();
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
    for (size_t i = sorted_blocks.size()-1; i > 0; i--) {
        Node::Ptr block = sorted_blocks[i];
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

    for (auto &block :  sorted_blocks) {
        if (block->immDom != block->semiDom && block->immDom != nullptr) {
            block->immDom = block->immDom->immDom;
        }
        if (block->immDom != nullptr) {
            output.emplace_back(std::make_pair(block->immDom, block));
        }
    }    
}

void Graph::SCC(std::vector< std::set<Node::Ptr> > &){

}

void Graph::link(Node::Ptr parent, Node::Ptr block) {
    Node::Ptr s = block;
    while (block->label->sdno() < s->child->label->sdno()) {
        if (s->size + s->child->child->size >= 2*s->child->size) {
            s->child->ancestor = s;
            s->child = s->child->child;
        } else {
            s->child->size = s->size;
            s->ancestor = s->child;
            s = s->child;
        }
    }

    s->label = block->label;
    parent->size += block->size;
    if (parent->size < 2 * block->size) {
        Node::Ptr tmp = s;
        s = parent->child;
        parent->child = tmp;
    }

    while (s != nullptr) {
        s->ancestor = parent;
        s = s->child;
    }
}

void Graph::DFS(Node::Ptr v, Graph::TraversalDirection dir) {
    v->dfs_no = currentDepthNo++;
    sorted_blocks.emplace_back(v);
    v->semiDom = v;
    const Node::EdgeList& edgelist = (dir == TraversalDirection::Natural) ?
        v->outEdgeList() : v->inEdgeList();
    for (const auto& succ : edgelist) {
        if (succ->dfs_no != -1) continue;
        succ->parent = v;
        DFS(succ, dir);
    }
}

} // namespace GraphAnalysis