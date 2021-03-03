#include "Graph.hpp"

namespace GraphAnalysis {

void Node::addInEdge(Node::Ptr n) {
    inEdges.emplace_back(n);
}

void Node::addOutEdge(Node::Ptr n) {
    outEdges.emplace_back(n);
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
void Graph::dominatorTree(EdgeList&) {
    
}

void Graph::postDominatorTree(EdgeList&) {

}
    
void Graph::SCC(std::vector< std::set<Node::Ptr> > &){

}

}