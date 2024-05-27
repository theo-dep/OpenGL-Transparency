// BSP functions    by Alan Baylis 2001
// https://www.alsprogrammingresource.com/bsp.html
// and https://github.com/GerardMT/BSP-Tree/tree/master

#pragma once

#include "Mesh.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <array>
#include <vector>

namespace bsp {

typedef std::array<Vertex, 3> Polygon;
typedef uint32_t NodePolygon;
typedef std::vector<NodePolygon> NodePolygonList;

struct Node {
    Node *l = nullptr;
    Node *r = nullptr;
    NodePolygonList pols;
    NodePolygon partition;
};

/**
 * Construct the BSP tree from the list of polygons provided.
 * @param polygons list of polygons.
 * @return the root node
 */
Node* Construct(const std::vector<Polygon> &polygons);
/**
 * Destroy the BSP tree (CPU and GPU data)
 */
void Destroy(Node *root);
/**
 * Render the BSP tree using the position
 */
void Render(const Node *root, const glm::mat4& viewMatrix);
/**
 * Return the number of nodes which compose the BSP tree.
 * @return number of nodes.
 */
std::size_t Nodes(const Node *root);
/**
 * Return the number of fragments (split planes) which have been generated while the construction of the BSP tree.
 * @return number of fragments.
 */
std::size_t Fragments(const Node *root);

}
