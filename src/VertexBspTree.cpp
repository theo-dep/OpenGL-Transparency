#include "VertexBspTree.hpp"

#include <iostream>
#include <fstream>

inline void writeNode(std::ofstream &ofs, const std::unique_ptr<VertexBspTreeType::Node> &node) noexcept;
inline std::unique_ptr<VertexBspTreeType::Node> readNode(std::ifstream &ifs) noexcept;

//--------------------------------------------------------------------------
VertexBspTree::VertexBspTree() : VertexBspTreeType(std::vector<Vertex>())
{
}

//--------------------------------------------------------------------------
VertexBspTree::VertexBspTree(std::vector<Vertex> && vertices, const std::vector<unsigned int> & indices)
    : VertexBspTreeType(std::move(vertices), indices)
{
}

//--------------------------------------------------------------------------
bool VertexBspTree::save(const std::string &filename) const noexcept
{
    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs)
    {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return false;
    }

    // Write vertices
    size_t verticesSize = vertices_.size();
    ofs.write(reinterpret_cast<const char*>(&verticesSize), sizeof(verticesSize));
    ofs.write(reinterpret_cast<const char*>(vertices_.data()), verticesSize * sizeof(Vertex));

    // Write BSP-tree
    writeNode(ofs, root_);

    ofs.close();

    return true;
}

//--------------------------------------------------------------------------
bool VertexBspTree::load(const std::string &filename) noexcept
{
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs)
    {
        std::cerr << "Failed to open file for reading: " << filename << std::endl;
        return false;
    }

    // Read vertices
    size_t verticesSize;
    ifs.read(reinterpret_cast<char*>(&verticesSize), sizeof(verticesSize));
    vertices_.resize(verticesSize);
    ifs.read(reinterpret_cast<char*>(vertices_.data()), verticesSize * sizeof(Vertex));

    // Read BSP-tree
    root_ = readNode(ifs);

    ifs.close();

    return true;
}


//--------------------------------------------------------------------------
inline void writeNode(std::ofstream &ofs, const std::unique_ptr<VertexBspTreeType::Node> &node) noexcept
{
    if (node)
    {
        // Write presence of node
        bool hasNode = true;
        ofs.write(reinterpret_cast<const char*>(&hasNode), sizeof(hasNode));

        // Write plane
        ofs.write(reinterpret_cast<const char*>(&std::get<0>(node->plane)), sizeof(VertexBspTreeType::point_type));
        ofs.write(reinterpret_cast<const char*>(&std::get<1>(node->plane)), sizeof(VertexBspTreeType::coord_type));

        // Write triangles
        size_t trianglesSize = node->triangles.size();
        ofs.write(reinterpret_cast<const char*>(&trianglesSize), sizeof(trianglesSize));
        ofs.write(reinterpret_cast<const char*>(node->triangles.data()), trianglesSize * sizeof(unsigned int));

        // Write child nodes
        writeNode(ofs, node->behind);
        writeNode(ofs, node->infront);
    }
    else
    {
        // Write absence of node
        bool hasNode = false;
        ofs.write(reinterpret_cast<const char*>(&hasNode), sizeof(hasNode));
    }
}

//--------------------------------------------------------------------------
inline std::unique_ptr<VertexBspTreeType::Node> readNode(std::ifstream &ifs) noexcept
{
    bool hasNode;
    ifs.read(reinterpret_cast<char*>(&hasNode), sizeof(hasNode));

    if (hasNode)
    {
        auto node = std::make_unique<VertexBspTreeType::Node>();

        // Read plane
        ifs.read(reinterpret_cast<char*>(&std::get<0>(node->plane)), sizeof(VertexBspTreeType::point_type));
        ifs.read(reinterpret_cast<char*>(&std::get<1>(node->plane)), sizeof(VertexBspTreeType::coord_type));

        // Read triangles
        size_t trianglesSize;
        ifs.read(reinterpret_cast<char*>(&trianglesSize), sizeof(trianglesSize));
        node->triangles.resize(trianglesSize);
        ifs.read(reinterpret_cast<char*>(node->triangles.data()), trianglesSize * sizeof(unsigned int));

        // Read child nodes
        node->behind = readNode(ifs);
        node->infront = readNode(ifs);

        return node;
    }
    else
    {
        return nullptr;
    }
}
