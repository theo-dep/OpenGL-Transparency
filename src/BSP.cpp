// BSP functions    by Alan Baylis 2001
// https://www.alsprogrammingresource.com/bsp.html
// and https://github.com/GerardMT/BSP-Tree/tree/master

#include "BSP.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/vec_swizzle.hpp>
#include <glm/gtx/intersect.hpp>

#include <algorithm>
#include <numeric>

namespace bsp {

GLuint g_bspVboId = 0;
GLuint g_bspVaoId = 0;

float g_minDistance = HUGE_VALF;

std::vector<Polygon> g_polygons; // saved polygons because nodes point to them

// Alan Baylis 2001
enum {On, Front, Back, TwoFrontOneBack, OneFrontTwoBack, OneFrontOneBack};
inline int ClassifyPoint(const glm::vec3& point, const glm::vec3& pO, const glm::vec3& pN);
inline int SplitPolygon(const Polygon& polygonToSplit, const Polygon& planePolygon, std::array<Polygon, 3>& polygons);

/**
 * Given a list of polygons, choose the polygon which defines the splitting plane.
 * @param polygons list of polygons.
 * @return index of the chosen polygon.
 */
inline std::size_t PolygonIndex(const NodePolygonList &polygons) {
    return rand() % polygons.size();
}

inline float minDistance(const std::vector<Polygon> &polygons) {
    float minDistance = HUGE_VALF;
    for (unsigned int i = 0; i < polygons.size(); ++i) {
        const glm::vec3& p1 = polygons[i][0].Position;
        const glm::vec3& p2 = polygons[i][1].Position;
        const glm::vec3& p3 = polygons[i][2].Position;

        minDistance = std::min(minDistance, glm::distance(p1, p2));
        minDistance = std::min(minDistance, glm::distance(p2, p3));
        minDistance = std::min(minDistance, glm::distance(p1, p3));
    }
    return minDistance;
}

/**
 * Recursive function. Construct the BSP tree from the list of polygons provided. The right subtree contains those polygons which are in front of the node plane,
 * while the left subtree contains those which are behind the node plane.
 * @param polygons list of polygons.
 * @param n node.
 */
inline void ConstructRec(const NodePolygonList &polygons, Node *n);
inline void BuildNormalsRec(Node *n);

Node* Construct(const std::vector<Polygon> &polygons) {
    if (polygons.empty()) {
        return nullptr;
    }

    g_minDistance = minDistance(polygons) / 2.f;

    Node* root = new Node;

    g_polygons = polygons;
    NodePolygonList iPolygons(polygons.size());
    std::iota(iPolygons.begin(), iPolygons.end(), 0);

    ConstructRec(iPolygons, root);

    // Normals
    BuildNormalsRec(root);

    glGenBuffers(1, &g_bspVboId);
    glGenVertexArrays(1, &g_bspVaoId);

    glBindVertexArray(g_bspVaoId);

    glBindBuffer(GL_ARRAY_BUFFER, g_bspVboId);
    glBufferData(GL_ARRAY_BUFFER, 3 * Fragments(root) * sizeof(Vertex), nullptr, GL_STREAM_DRAW);

    return root;
}

inline NodePolygon PushBack(const Polygon& polygon) {
    g_polygons.push_back(polygon);
    return (g_polygons.size() - 1);
}

inline void ConstructRec(const NodePolygonList &polygons, Node *n) {
    unsigned int pol_i = PolygonIndex(polygons);
    n->pols.push_back(polygons[pol_i]);
    n->partition = polygons[pol_i];

    NodePolygonList polygons_front;
    NodePolygonList polygons_back;

    for (unsigned int i = 0; i < polygons.size(); ++i) {
        if (i != pol_i) {
            n->pols.push_back(polygons[i]);

            std::array<Polygon, 3> output;
            switch (SplitPolygon(g_polygons[polygons[i]], g_polygons[n->partition], output))
            {
                case On:
                break;

                case Front:
                    polygons_front.push_back(polygons[i]);
                break;

                case Back:
                    polygons_back.push_back(polygons[i]);
                break;

                case TwoFrontOneBack:
                    polygons_front.push_back(PushBack(output[0]));
                    polygons_front.push_back(PushBack(output[1]));
                    polygons_back.push_back(PushBack(output[2]));
                break;

                case OneFrontTwoBack:
                    polygons_front.push_back(PushBack(output[0]));
                    polygons_back.push_back(PushBack(output[1]));
                    polygons_back.push_back(PushBack(output[2]));
                break;

                case OneFrontOneBack:
                    polygons_front.push_back(PushBack(output[0]));
                    polygons_back.push_back(PushBack(output[1]));
                break;
            }
        }
    }

    if (!polygons_front.empty()) {
        n->r = new Node;
        ConstructRec(polygons_front, n->r);
    }

    if (!polygons_back.empty()) {
        n->l = new Node;
        ConstructRec(polygons_back, n->l);
    }
}

inline void BuildNormalsRec(Node *n) {
    if (n != nullptr) {
        for (const NodePolygon& polygon : n->pols)
        {
            glm::vec3 edge1 = g_polygons[polygon][1].Position - g_polygons[polygon][0].Position;
            glm::vec3 edge2 = g_polygons[polygon][2].Position - g_polygons[polygon][0].Position;
            glm::vec3 polysNormal = glm::normalize(glm::cross(edge1, edge2));
            g_polygons[polygon][0].Normal += polysNormal;
            g_polygons[polygon][1].Normal += polysNormal;
            g_polygons[polygon][2].Normal += polysNormal;
        }

        BuildNormalsRec(n->l);
        BuildNormalsRec(n->r);
    }
}

/**
 * Delete recursively all tree nodes. Including <b>n</b>.
 * @param n tree root node.
 */
inline void DestroyRec(Node *n);

void Destroy(Node *root) {
    glDeleteBuffers(1, &g_bspVboId);
    glDeleteVertexArrays(1, &g_bspVaoId);

    DestroyRec(root);
}

inline void DestroyRec(Node *n) {
    if (n != nullptr)
    {
        DestroyRec(n->l);
        DestroyRec(n->r);

        delete n;
    }
}

/**
 * Render recursively all tree nodes. Including <b>n</b>.
 * @param n tree root node.
 * @param modelViewProjectionMatrix
 * @param vertices list of polygons back to front
 */
inline void RenderRec(const Node *n, const glm::mat4& modelViewProjectionMatrix, std::vector<Vertex> &vertices);

void Render(const Node *root, const glm::mat4& modelViewProjectionMatrix) {
    std::vector<Vertex> vertices;
    RenderRec(root, modelViewProjectionMatrix, vertices);

    glBindVertexArray(g_bspVaoId);

    glBindBuffer(GL_ARRAY_BUFFER, g_bspVboId);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(Vertex), &vertices[0]);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLubyte*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLubyte*)offsetof(Vertex, Normal));

    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
}

inline void RenderRec(const Node *n, const glm::mat4& modelViewProjectionMatrix, std::vector<Vertex> &vertices) {
    if (n == nullptr)
    {
        return;
    }

    if (n->l != nullptr && n->r != nullptr)
    {
        // get the partitioning planes normal
        const glm::vec3& p1 = g_polygons[n->partition][0].Position;
        const glm::vec3& p2 = g_polygons[n->partition][1].Position;
        const glm::vec3& p3 = g_polygons[n->partition][2].Position;

        glm::vec3 edge1 = p2 - p1;
        glm::vec3 edge2 = p3 - p1;
        glm::vec3 planeNormal = glm::cross(edge1, edge2);
        glm::vec3 temp = p1;

        //The current position of the player/viewpoint
        glm::vec3 position = glm::xyz(glm::column(modelViewProjectionMatrix, 3));
        int side = ClassifyPoint(position, temp, planeNormal);

        if (side == -1)
        {
            RenderRec(n->r, modelViewProjectionMatrix, vertices);
            RenderRec(n->l, modelViewProjectionMatrix, vertices);
        }
        else
        {
            RenderRec(n->l, modelViewProjectionMatrix, vertices);
            RenderRec(n->r, modelViewProjectionMatrix, vertices);
        }
    }
    else //if (n->l == nullptr || n->r == nullptr)
    {
        //Draw polygons that are in the leaf
        for (const NodePolygon& polygon : n->pols)
        {
            for (const Vertex& vertex : g_polygons[polygon])
            {
                vertices.push_back(vertex);
            }
        }
    }
}

inline int ClassifyPoint(const glm::vec3& point, const glm::vec3& pO, const glm::vec3& pN)
{
    glm::vec3 dir = pO - point;
    float d = glm::dot(dir, pN);

    return (d < -0.001f ? 1 :
            d > 0.001f ? -1 : 0);
}

/*
 Although this function is called SplitPolygon, it really only splits triangles, the
 function inputs the triangle to be split and another triangle to be used for the splitting
 plane. The parameter 'polygons' is a pointer to 3 triangles for output.
 The function splits the input triangle into either 0, 2 or 3 new triangles with
 recalculated texture coordinates.
 If the polygons pointer happens to be NULL then the function will just set the outputFlag and return.

 The return value will be either Front, Back, TwoFrontOneBack, OneFrontTwoBack or OneFrontOneBack.
 Front means that all points are infront of the plane or the polygon lies on the plane and faces the front.
 Back means that all points are behind of the plane or the polygon lies on the plane and faces the back.
 TwoFrontOneBack means that polygons 1 and 2 are infront of the plane and polygon 3 is behind.
 OneFrontTwoBack means that polygon 1 is infront of the plane and polygons 2 and 3 are behind.
 OneFrontOneBack means that polygon 1 is infront of the plane and polygon 2 is behind, polygon 3 is not used.
*/
inline int SplitPolygon(const Polygon& polygonToSplit, const Polygon& planePolygon, std::array<Polygon, 3>& polygons)
{
    // make a distance limit between 2 points to avoid infinite split
    const glm::vec3& p1 = polygonToSplit[0].Position;
    const glm::vec3& p2 = polygonToSplit[1].Position;
    const glm::vec3& p3 = polygonToSplit[2].Position;
    if (glm::distance(p1, p2) < g_minDistance ||
        glm::distance(p1, p3) < g_minDistance ||
        glm::distance(p2, p3) < g_minDistance)
    {
        return On;
    }

    // get a point on the plane
    glm::vec3 pointOnPlane = planePolygon[0].Position;

    // get the splitting planes normal
    glm::vec3 edge1 = planePolygon[1].Position - planePolygon[0].Position;
    glm::vec3 edge2 = planePolygon[2].Position - planePolygon[0].Position;
    glm::vec3 planeNormal = glm::cross(edge1, edge2);

    // get the normal of the polygon to split
    edge1 = p2 - p1;
    edge2 = p3 - p1;
    glm::vec3 polysNormal = glm::cross(edge1, edge2);

    // check if the polygon lies on the plane
    int count = 0;
    for (const Vertex& vertexToSplit : polygonToSplit)
    {
        if (ClassifyPoint(vertexToSplit.Position, pointOnPlane, planeNormal) == 0)
            count++;
        else
            break;
    }
    if (count == 3)
    {
        if (ClassifyPoint(polysNormal, pointOnPlane, planeNormal) == 1)
            return Front;
        if (ClassifyPoint(polysNormal, pointOnPlane, planeNormal) == -1)
            return Back;
    }

    // try to split the polygon
    float intersectionDistance;
    glm::vec3 ptA = p3, ptB, edgeDirection, intersection;
    int sideB, sideA = ClassifyPoint(ptA, pointOnPlane, planeNormal);
    std::size_t out_c = 0, in_c = 0;
    std::array<glm::vec3, 4> outpts, inpts;
    for (const Vertex& vertexToSplit : polygonToSplit)
    {
        ptB = vertexToSplit.Position;
        edgeDirection = ptB - ptA;
        sideB = ClassifyPoint(ptB, pointOnPlane, planeNormal);
        if (sideB > 0)
        {
            if (sideA < 0)
            {
                // find intersection
                if (glm::intersectRayPlane(ptA, edgeDirection, pointOnPlane, planeNormal, intersectionDistance)) {
                    intersection = ptA + intersectionDistance * edgeDirection;
                    outpts[out_c++] = inpts[in_c++] = intersection;
                }
            }
            inpts[in_c++] = ptB;
        }
        else if (sideB < 0)
        {
            if (sideA > 0)
            {
                // find intersection
                if (glm::intersectRayPlane(ptA, edgeDirection, pointOnPlane, planeNormal, intersectionDistance)) {
                    intersection = ptA + intersectionDistance * edgeDirection;
                    outpts[out_c++] = inpts[in_c++] = intersection;
                }
            }
            outpts[out_c++] = ptB;
        }
        else
        {
            outpts[out_c++] = inpts[in_c++] = ptB;
        }

        ptA = ptB;
        sideA = sideB;
    }

    int outputFlag;
    if (in_c == 4)          // two polygons are infront, one behind
    {
        outputFlag = TwoFrontOneBack;

        polygons[0][0].Position = inpts[0];
        polygons[0][1].Position = inpts[1];
        polygons[0][2].Position = inpts[2];
        polygons[1][0].Position = inpts[0];
        polygons[1][1].Position = inpts[2];
        polygons[1][2].Position = inpts[3];
        polygons[2][0].Position = outpts[0];
        polygons[2][1].Position = outpts[1];
        polygons[2][2].Position = outpts[2];
    }
    else if (out_c == 4)    // one polygon is infront, two behind
    {
        outputFlag = OneFrontTwoBack;

        polygons[0][0].Position = inpts[0];
        polygons[0][1].Position = inpts[1];
        polygons[0][2].Position = inpts[2];
        polygons[1][0].Position = outpts[0];
        polygons[1][1].Position = outpts[1];
        polygons[1][2].Position = outpts[2];
        polygons[2][0].Position = outpts[0];
        polygons[2][1].Position = outpts[2];
        polygons[2][2].Position = outpts[3];
    }
    else if (in_c == 3 && out_c == 3)  // plane bisects the polygon
    {
        outputFlag = OneFrontOneBack;

        polygons[0][0].Position = inpts[0];
        polygons[0][1].Position = inpts[1];
        polygons[0][2].Position = inpts[2];
        polygons[1][0].Position = outpts[0];
        polygons[1][1].Position = outpts[1];
        polygons[1][2].Position = outpts[2];
    }
    else // then polygon must be totally infront of or behind the plane
    {
        int side;

        for (const Vertex& vertexToSplit : polygonToSplit)
        {
            side = ClassifyPoint(vertexToSplit.Position, pointOnPlane, planeNormal);
            if (side == 1)
            {
                outputFlag = Front;
                break;
            }
            else if (side == -1)
            {
                outputFlag = Back;
                break;
            }
        }
    }

    // Normals to be calculated later
    for (Polygon& polygon : polygons)
    {
        polygon[2].Normal = polygon[1].Normal = polygon[0].Normal = glm::vec3(0);
    }

    return outputFlag;
}

inline std::size_t NodesRec(const Node *n)
{
    if (n == nullptr)
    {
        return 0;
    }

    std::size_t nodes = 1;
    nodes += NodesRec(n->l);
    nodes += NodesRec(n->r);
    return nodes;
}

std::size_t Nodes(const Node *root)
{
    return NodesRec(root);
}

inline std::size_t FragmentsRec(const Node *n)
{
    if (n == nullptr)
    {
        return 0;
    }

    std::size_t fragments = n->pols.size();
    fragments += FragmentsRec(n->l);
    fragments += FragmentsRec(n->r);
    return fragments;
}

std::size_t Fragments(const Node *root)
{
    return FragmentsRec(root);
}

}
