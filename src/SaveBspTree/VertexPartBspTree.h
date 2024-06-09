#pragma once

#include "VertexBspTree.hpp"

// From https://github.com/roever/BSP/tree/master
class VertexPartBspTree : public VertexBspTree
{
public:
    VertexPartBspTree();
    VertexPartBspTree(std::vector<Vertex> && vertices, const std::vector<unsigned int> & indices);
    VertexPartBspTree(std::vector<Vertex> && vertices);

    /// perform boolean operations on the polyhedra defined by the two trees
    static std::shared_ptr<VertexPartBspTree> unify(const VertexPartBspTree & lhs, const VertexPartBspTree & rhs);

protected:
    // append vertices
    void append(std::vector<Vertex> & v, const Vertex & v1, const Vertex & v2, const Vertex & v3) const;

    // classify the triangles of the tree given as parameter and keep the
    // polygons that are choosen and put them into out vector
    // tree: tree to classify
    // inside: true: keep inside polygons, else keep outside polygons
    //    when inside polygons are kept they are automatically flipped
    // keepEdge: keep faces that are on the edge of the other polyhedron
    // out: where to put the triangles
    void classifyTree(const Node * tree, const std::vector<Vertex> & vertices, bool inside, bool keepEdge, std::vector<Vertex> & out) const;

    // assumes, that the node is not empty
    bool classifyTriangle(const Node * n, const Vertex & v1, const Vertex & v2, const Vertex & v3, bool inside, bool keepEdge, std::vector<Vertex> & out, bool keepNow) const;
};
