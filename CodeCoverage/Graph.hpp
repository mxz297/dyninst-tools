#ifndef GRAPH_HPP
#define GRAPH_HPP

#include <vector>
#include <set>
#include <memory>

namespace GraphAnalysis {

class Node {
    friend class Graph;
public:
    using Ptr = std::shared_ptr<Node>;
    using EdgeList = std::vector<Ptr>;
    using EdgeFuncPtr = const EdgeList& (*)();
    void addInEdge(Ptr);
    void addOutEdge(Ptr);
    const EdgeList& inEdgeList() { return inEdges; }
    const EdgeList& outEdgeList() { return outEdges; }

    int sdno();
    void compress();
    Ptr eval();    
    void clearDominatorInfo();
protected:    
    EdgeList outEdges, inEdges;
    
    int dfs_no;
    int size;
    Ptr semiDom, immDom, label, ancestor, parent, child;
    std::set<Ptr> bucket;
};

class Graph {

public:
    using Ptr = std::shared_ptr<Graph>;
    using NodeList = std::vector<Node::Ptr>;
    using EdgeList = std::vector< std::pair< Node::Ptr, Node::Ptr> >;
    void addNode(Node::Ptr);
    void addEntry(Node::Ptr);
    void addExit(Node::Ptr);

    void dominatorTree(EdgeList&);
    void postDominatorTree(EdgeList&);
    void SCC(std::vector< std::set<Node::Ptr> > &);
protected:
    enum class TraversalDirection { Natural, Reverse };
    void link(Node::Ptr parent, Node::Ptr block);
    void DFS(Node::Ptr, TraversalDirection);
    void initializeDominatorInfo();
    void dominatorComputation(EdgeList& output, Graph::TraversalDirection dir);

    std::vector<Node::Ptr> naturalOrder, reverseOrder;
    int currentDepthNo;
    
    NodeList allNodes;
    NodeList entries;
    NodeList exits;    
};

}
#endif