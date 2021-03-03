#ifndef GRAPH_HPP
#define GRAPH_HPP

#include <vector>
#include <set>
#include <memory>

namespace GraphAnalysis {

class Node {
public:
    using Ptr = std::shared_ptr<Node>;
    using EdgeList = std::vector<Ptr>;
    void addInEdge(Ptr);
    void addOutEdge(Ptr);
    const EdgeList& inEdgeList() { return inEdges; }
    const EdgeList& outEdgeList() { return outEdges; }    
private:    
    EdgeList outEdges, inEdges;  
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
private:
    NodeList allNodes;
    NodeList entries;
    NodeList exits;    
};

}
#endif