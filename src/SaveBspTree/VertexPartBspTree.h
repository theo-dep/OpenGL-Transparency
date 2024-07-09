#pragma once

#include "VertexBspTree.hpp"

class VertexPartBspTree : public VertexBspTree
{
public:
    VertexPartBspTree();
    VertexPartBspTree(std::vector<Vertex> && vertices, const std::vector<unsigned int> & indices);

    // lhs as behind, rhs as infront if lhs plane normal is behind rhs plane normal
    // otherwise rhs as behind and lhs as infront
    static std::shared_ptr<VertexPartBspTree> merge(const VertexPartBspTree & lhs, const VertexPartBspTree & rhs);

protected:
    // copy node into the tree
    std::unique_ptr<Node> copy(const Node * n, const std::vector<Vertex> & v);
};
