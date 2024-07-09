#include "VertexPartBspTree.h"

#include <unordered_map>

//--------------------------------------------------------------------------
VertexPartBspTree::VertexPartBspTree() : VertexBspTree()
{
}

//--------------------------------------------------------------------------
VertexPartBspTree::VertexPartBspTree(std::vector<Vertex> && vertices, const std::vector<unsigned int> & indices) : VertexBspTree(std::move(vertices), indices)
{
}

//--------------------------------------------------------------------------
inline glm::vec3 centroid(const std::vector<Vertex> & vertices)
{
    glm::vec3 sum(0.0f);
    for (const Vertex & vertex : vertices)
    {
        sum += vertex.Position;
    }
    const glm::vec3 centroid{ sum / static_cast<float>(vertices.size()) };
    return centroid;
}

//--------------------------------------------------------------------------
std::shared_ptr<VertexPartBspTree> VertexPartBspTree::merge(const VertexPartBspTree & lhs, const VertexPartBspTree & rhs)
{
    // Find the equidistant plane between two sets of points
    const glm::vec3 centroidL{ centroid(lhs.vertices_) };
    const glm::vec3 centroidR{ centroid(rhs.vertices_) };
    const glm::vec3 normal{ glm::normalize(centroidL - centroidR) };
    const glm::vec3 origin{ (centroidL + centroidR) / 2.f };
    const float offset{ glm::dot(normal, origin) };
    const Plane plane{ std::make_tuple(normal, offset) };

    std::shared_ptr tree{ std::make_shared<VertexPartBspTree>() };
    tree->root_ = std::make_unique<Node>();
    tree->root_->plane = plane;

    if (glm::dot(centroidR, centroidL) < 0)
    {
        tree->root_->behind = tree->copy(lhs.root_.get(), lhs.vertices_);
        tree->root_->infront = tree->copy(rhs.root_.get(), rhs.vertices_);
    }
    else
    {
        tree->root_->behind = tree->copy(rhs.root_.get(), rhs.vertices_);
        tree->root_->infront = tree->copy(lhs.root_.get(), lhs.vertices_);
    }

    return tree;
}

//--------------------------------------------------------------------------
std::unique_ptr<VertexPartBspTree::Node> VertexPartBspTree::copy(const Node * n, const std::vector<Vertex> & v)
{
    if (n)
    {
        std::unique_ptr node{ std::make_unique<Node>() };
        node->plane = n->plane;

        std::unordered_map<unsigned int, unsigned int> mergedIndices;
        for (unsigned int index : n->triangles)
        {
            if (mergedIndices.contains(index))
            {
                node->triangles.push_back(mergedIndices[index]);
            }
            else
            {
                mergedIndices.emplace(index, static_cast<unsigned int>(vertices_.size()));
                node->triangles.push_back(vertices_.size());
                vertices_.push_back(v[index]);
            }
        }

        node->behind = copy(n->behind.get(), v);
        node->infront = copy(n->infront.get(), v);

        return node;
    }

    return std::unique_ptr<Node>();
}
