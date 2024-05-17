// BSP functions    by Alan Baylis 2001
// https://www.alsprogrammingresource.com/bsp.html
// and https://github.com/GerardMT/BSP-Tree/tree/master

#include "BSP.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/vec_swizzle.hpp>

#include <algorithm>

GLuint g_bspVboId = 0;
GLuint g_bspVaoId = 0;

// Alan Baylis 2001
enum {Front, Back, TwoFrontOneBack, OneFrontTwoBack, OneFrontOneBack};
inline int ClassifyPoint(const glm::vec3& point, const glm::vec3& pO, const glm::vec3& pN);
inline glm::vec3 GetEdgeIntersection(const glm::vec3& point0, const glm::vec3& point1, const bsp_tree::polygon& planePolygon);
inline int SplitPolygon(const bsp_tree::polygon& polygonToSplit, const bsp_tree::polygon& planePolygon, std::array<bsp_tree::polygon, 3>& polygons);
#define USE_AB2001

unsigned int bsp_tree::polygon_index(const std::vector<polygon> &polygons) const {
    return rand() % polygons.size();
}

void bsp_tree::construct(const std::vector<polygon> &polygons) {
    if (polygons.empty()) {
        return;
    }

    fragments_ = 0;

    root_ = new node;
    nodes_ = 1;
    std::vector erasedPolygons = polygons;
    construct_rec(erasedPolygons, root_);

    glGenBuffers(1, &g_bspVboId);
    glGenVertexArrays(1, &g_bspVaoId);

    glBindVertexArray(g_bspVaoId);

    glBindBuffer(GL_ARRAY_BUFFER, g_bspVboId);
    glBufferData(GL_ARRAY_BUFFER, 3 * fragments() * sizeof(Vertex), nullptr, GL_STREAM_DRAW);
}

void bsp_tree::construct_rec(/*const*/ std::vector<polygon> &polygons, node *n) {
    unsigned int pol_i = polygon_index(polygons);
    n->pols.push_back(polygons[pol_i]);
    n->partition = polygons[pol_i];

    to_plane(polygons[pol_i], n->plane);

    std::vector<polygon> polygons_front;
    std::vector<polygon> polygons_back;

    for (unsigned int i = 0; i < polygons.size(); ++i) {
        if (i != pol_i) {
#ifdef USE_AB2001
            n->pols.push_back(polygons[i]);

            std::array<polygon, 3> output;
            switch (SplitPolygon(polygons[i], n->partition, output))
            {
                case Front:
                    polygons_front.push_back(polygons[i]);
                break;

                case Back:
                    polygons_back.push_back(polygons[i]);
                break;

                case TwoFrontOneBack:
                    polygons_front.push_back(output[0]);
                    polygons_front.push_back(output[1]);
                    polygons_back.push_back(output[2]);
                break;

                case OneFrontTwoBack:
                    polygons_front.push_back(output[0]);
                    polygons_back.push_back(output[1]);
                    polygons_back.push_back(output[2]);
                break;

                case OneFrontOneBack:
                    polygons_front.push_back(output[0]);
                    polygons_back.push_back(output[1]);
                break;
            }
#else
            switch (distance(n->plane, polygons[i])) {
                case ON:
                    n->pols.push_back(polygons[i]);
                    break;
                case FRONT:
                    polygons_front.push_back(polygons[i]);
                    break;

                case BACK:
                    polygons_back.push_back(polygons[i]);
                    break;

                case HALF:
                    polygon_split(n->plane, polygons[i], polygons_front, polygons_back);
                    break;
            }
#endif
        }
    }

#ifdef USE_AB2001
    // clean memory because all model points are copied in front or back
    polygons.clear();
    polygons.shrink_to_fit();
    // do not use it after construct_rec
#endif

    fragments_ += n->pols.size();

    if (!polygons_front.empty()) {
        n->r = new node;
        ++nodes_;
        construct_rec(polygons_front, n->r);
    } else {
        n->r = nullptr;
    }

    if (!polygons_back.empty()) {
        n->l = new node;
        ++nodes_;
        construct_rec(polygons_back, n->l);
    } else {
        n->l = nullptr;
    }
}

void bsp_tree::to_plane(const polygon &pol, glm::vec4 &pl) const {
    glm::vec3 p1 = pol.v[0].Position;
    glm::vec3 p2 = pol.v[1].Position;
    glm::vec3 p3 = pol.v[2].Position;

    glm::vec3 u = p2 - p1;
    glm::vec3 v = p3 - p1;

    glm::vec3 r = glm::cross(u, v);
    pl.x = r.x;
    pl.y = r.y;
    pl.z = r.z;
    pl.w = -glm::dot(glm::xyz(pl), p1);
}

bsp_tree::dist_res bsp_tree::distance(const glm::vec4 &pl, const polygon &pol) const {
    glm::vec3 p1 = pol.v[0].Position;
    glm::vec3 p2 = pol.v[1].Position;
    glm::vec3 p3 = pol.v[2].Position;

    float d1 = glm::dot(pl, glm::vec4(p1, 1));
    float d2 = glm::dot(pl, glm::vec4(p2, 1));

    if (d1 < 0 && d2 > 0) {
        return bsp_tree::HALF;
    } else {
        float d3 = glm::dot(pl, glm::vec4(p3, 1));

        if (d3 < 0) {
            return bsp_tree::BACK;
        } else if (d3 > 0) {
            return bsp_tree::FRONT;
        } else if (d1 == 0 && d2 == 0 && d3 == 0) {
            return bsp_tree::ON;
        }
    }

    //unreachable
    return static_cast<dist_res>(-1);
}

void bsp_tree::plane_segment_intersection(const glm::vec4 &pl, const glm::vec3 &a, const glm::vec3 &b, glm::vec3 &i) const {
    float t = glm::dot(pl, glm::vec4(a, 1)) / glm::dot(glm::xyz(pl), b - a);
    i = a + t * (a - b);
}

void bsp_tree::polygon_split_aux(const glm::vec4 &pl, const glm::vec3 &a, const glm::vec3 &b1, const glm::vec3 &b2, std::vector<polygon> &polygons_a, std::vector<polygon> &polygons_b) {
    glm::vec3 i_ab1;
    plane_segment_intersection(pl, a, b1, i_ab1);

    glm::vec3 i_ab2;
    plane_segment_intersection(pl, a, b2, i_ab2);

#define ADD_VERTEX(Poly, Point, PointNormal, Index) \
    Poly.v[Index].Position = Point; \
    Poly.v[Index].Normal = PointNormal

    polygon p_a;
    glm::vec3 n_a = glm::normalize(glm::cross(i_ab1 - a, i_ab2 - a));
    ADD_VERTEX(p_a, a, n_a, 0);
    ADD_VERTEX(p_a, i_ab1, n_a, 1);
    ADD_VERTEX(p_a, i_ab2, n_a, 2);
    polygons_a.push_back(p_a);

    polygon p_b1;
    glm::vec3 n_b1 = glm::normalize(glm::cross(i_ab2 - b1, i_ab1 - b1));
    ADD_VERTEX(p_b1, b1, n_b1, 0);
    ADD_VERTEX(p_b1, i_ab2, n_b1, 1);
    ADD_VERTEX(p_b1, i_ab1, n_b1, 2);
    polygons_b.push_back(p_b1);

    polygon p_b2;
    glm::vec3 n_b2 = glm::normalize(glm::cross(b2 - b1, i_ab2 - b1));
    ADD_VERTEX(p_b2, b1, n_b2, 0);
    ADD_VERTEX(p_b2, b2, n_b2, 1);
    ADD_VERTEX(p_b2, i_ab2, n_b2, 2);
    polygons_b.push_back(p_b2);
}

void bsp_tree::polygon_split(const glm::vec4 &pl, const polygon &pol, std::vector<polygon> &polygons_front, std::vector<polygon> &polygons_back) {
    glm::vec3 p1 = pol.v[0].Position;
    glm::vec3 p2 = pol.v[1].Position;
    glm::vec3 p3 = pol.v[2].Position;

    float d1 = glm::dot(pl, glm::vec4(p1, 1));
    float d2 = glm::dot(pl, glm::vec4(p2, 1));
    float d3 = glm::dot(pl, glm::vec4(p3, 1));

    if (d1 < 0 && d2 > 0 && d3 > 0) {
        polygon_split_aux(pl, p1, p2, p3, polygons_front, polygons_back);
    } else if (d2 < 0 && d1 > 0 && d3 > 0) {
        polygon_split_aux(pl, p2, p1, p3, polygons_front, polygons_back);
    } else if (d3 < 0 && d1 > 0 && d2 > 0) {
        polygon_split_aux(pl, p3, p1, p2, polygons_front, polygons_back);
    } else if (d1 > 0 && d2 < 0 && d3 < 0) {
        polygon_split_aux(pl, p1, p2, p3, polygons_back, polygons_front);
    } else if (d2 > 0 && d1 < 0 && d3 < 0) {
        polygon_split_aux(pl, p2, p1, p3, polygons_back, polygons_front);
    } else if (d3 > 0 && d1 < 0 && d2 < 0) {
        polygon_split_aux(pl, p1, p2, p3, polygons_back, polygons_front);
    }
}

void bsp_tree::destroy() {
    glDeleteBuffers(1, &g_bspVboId);
    glDeleteVertexArrays(1, &g_bspVaoId);

    erase_rec(root_);
}

void bsp_tree::erase_rec(node *n) {
    if (n->l != nullptr) {
        erase_rec(n->l);
    }

    if (n->r != nullptr) {
        erase_rec(n->r);
    }

    delete n;
}

unsigned int bsp_tree::nodes() {
    return nodes_;
}

unsigned int bsp_tree::fragments() {
    return fragments_;
}

void bsp_tree::render(const glm::mat4& modelViewProjectionMatrix) {
    std::vector<Vertex> vertices;
    render_rec(root_, modelViewProjectionMatrix, vertices);

    glBindVertexArray(g_bspVaoId);

    glBindBuffer(GL_ARRAY_BUFFER, g_bspVboId);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(Vertex), &vertices[0]);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLubyte*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLubyte*)offsetof(Vertex, Normal));

    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
}

void bsp_tree::render_rec(node *n, const glm::mat4& modelViewProjectionMatrix, std::vector<Vertex> &vertices) {
    const auto transform {
        [&vertices, &n]() -> void
        {
            for (const polygon& polygon : n->pols)
            {
                for (const Vertex& vertex : polygon.v)
                {
                    vertices.push_back(vertex);
                }
            }
        }
    };

    if (n->l != nullptr && n->r != nullptr)
    {
        // get the partitioning planes normal
#ifdef USE_AB2001
        glm::vec3 edge1 = n->partition.v[1].Position - n->partition.v[0].Position;
        glm::vec3 edge2 = n->partition.v[2].Position - n->partition.v[0].Position;
        glm::vec3 planeNormal = glm::cross(edge1, edge2);
        glm::vec3 temp = n->partition.v[0].Position;
#else
        glm::vec3 planeNormal = glm::xyz(n->plane);
        glm::vec3 temp = glm::xyz(n->pols[0].v[0].Position);
#endif
        //The current position of the player/viewpoint
        glm::vec3 position = glm::xyz(glm::column(modelViewProjectionMatrix, 3));
        int side = ClassifyPoint(position, temp, planeNormal);

        if (side == -1)
        {
            render_rec(n->r, modelViewProjectionMatrix, vertices);
            render_rec(n->l, modelViewProjectionMatrix, vertices);
        }
        else
        {
            render_rec(n->l, modelViewProjectionMatrix, vertices);
            render_rec(n->r, modelViewProjectionMatrix, vertices);
        }

#ifndef USE_AB2001
        transform();
#endif
    }
    else //if (n->l == nullptr || n->r == nullptr)
    {
        //Draw polygons that are in the leaf
        transform();

#ifndef USE_AB2001
        if (n->l != nullptr)
        {
            render_rec(n->l, modelViewProjectionMatrix, vertices);
        }
        if (n->r != nullptr)
        {
            render_rec(n->r, modelViewProjectionMatrix, vertices);
        }
#endif
    }
}

inline int ClassifyPoint(const glm::vec3& point, const glm::vec3& pO, const glm::vec3& pN)
{
    glm::vec3 dir = pO - point;
    float d = glm::dot(dir, pN);

    return (d < -0.001f ? 1 :
            d > 0.001f ? -1 : 0);
}

inline glm::vec3 GetEdgeIntersection(const glm::vec3& point0, const glm::vec3& point1, const bsp_tree::polygon& planePolygon)
{
    // get a point on the plane
    glm::vec3 pointOnPlane = planePolygon.v[0].Position;

    // get the splitting planes normal
    glm::vec3 edge1 = planePolygon.v[1].Position - planePolygon.v[0].Position;
    glm::vec3 edge2 = planePolygon.v[2].Position - planePolygon.v[0].Position;
    glm::vec3 planeNormal = glm::cross(edge1, edge2);

// find edge intersection:
// intersection = p0 + (p1 - p0) * t
// where t = (planeNormal . (pointOnPlane - p0)) / (planeNormal . (p1 - p0))

    //planeNormal . (pointOnPlane - point0)
    glm::vec3 temp = pointOnPlane- point0;
    float numerator = glm::dot(planeNormal, temp);

    //planeNormal . (point1 - point0)
    temp = point1 - point0;
    float denominator = glm::dot(planeNormal, temp);

    float t = denominator > 0 ? numerator / denominator : 0;

    glm::vec3 intersection = point0 + temp * t;
    return intersection;
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
inline int SplitPolygon(const bsp_tree::polygon& polygonToSplit, const bsp_tree::polygon& planePolygon, std::array<bsp_tree::polygon, 3>& polygons)
{
    // make a distance limit between 2 points to avoid infinite split
    //std::cout << glm::distance(polygonToSplit.v[0].Position, polygonToSplit.v[1].Position) << std::endl;
    if (glm::distance(polygonToSplit.v[0].Position, polygonToSplit.v[1].Position) < 0.0025f)
        return -1;

    // get a point on the plane
    glm::vec3 pointOnPlane = planePolygon.v[0].Position;

    // get the splitting planes normal
    glm::vec3 edge1 = planePolygon.v[1].Position - planePolygon.v[0].Position;
    glm::vec3 edge2 = planePolygon.v[2].Position - planePolygon.v[0].Position;
    glm::vec3 planeNormal = glm::cross(edge1, edge2);

    // get the normal of the polygon to split
    edge1 = polygonToSplit.v[1].Position - polygonToSplit.v[0].Position;
    edge2 = polygonToSplit.v[2].Position - polygonToSplit.v[0].Position;
    glm::vec3 polysNormal = glm::cross(edge1, edge2);

    // check if the polygon lies on the plane
    glm::vec3 temp;
    int count = 0;
    for (int loop = 0; loop < 3; loop++)
    {
        temp = polygonToSplit.v[loop].Position;
        if (ClassifyPoint(temp, pointOnPlane, planeNormal) == 0)
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
    glm::vec3 ptA = polygonToSplit.v[2].Position;
    temp = ptA;
    int sideB, sideA = ClassifyPoint(temp, pointOnPlane, planeNormal);
    glm::vec3 ptB, intersection;
    std::size_t out_c = 0, in_c = 0;
    std::array<glm::vec3, 4> outpts, inpts;
    for (int loop = 0; loop < 3; loop++)
    {
        ptB = polygonToSplit.v[loop].Position;
        temp = ptB;
        sideB = ClassifyPoint(temp, pointOnPlane, planeNormal);
        if (sideB > 0)
        {
            if (sideA < 0)
            {
                // find intersection
                edge1.x = ptA.x;
                edge1.y = ptA.y;
                edge1.z = ptA.z;
                edge2.x = ptB.x;
                edge2.y = ptB.y;
                edge2.z = ptB.z;

                temp = GetEdgeIntersection(edge1, edge2, planePolygon);
                intersection = temp;

                outpts[out_c++] = inpts[in_c++] = intersection;
            }
            inpts[in_c++] = ptB;
        }
        else if (sideB < 0)
        {
            if (sideA > 0)
            {
                // find intersection
                edge1 = ptA;
                edge2 = ptB;

                temp = GetEdgeIntersection(edge1, edge2, planePolygon);
                intersection = temp;

                outpts[out_c++] = inpts[in_c++] = intersection;
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

        polygons[0].v[0].Position = inpts[0];
        polygons[0].v[1].Position = inpts[1];
        polygons[0].v[2].Position = inpts[2];
        polygons[1].v[0].Position = inpts[0];
        polygons[1].v[1].Position = inpts[2];
        polygons[1].v[2].Position = inpts[3];
        polygons[2].v[0].Position = outpts[0];
        polygons[2].v[1].Position = outpts[1];
        polygons[2].v[2].Position = outpts[2];
    }
    else if (out_c == 4)    // one polygon is infront, two behind
    {
        outputFlag = OneFrontTwoBack;

        polygons[0].v[0].Position = inpts[0];
        polygons[0].v[1].Position = inpts[1];
        polygons[0].v[2].Position = inpts[2];
        polygons[1].v[0].Position = outpts[0];
        polygons[1].v[1].Position = outpts[1];
        polygons[1].v[2].Position = outpts[2];
        polygons[2].v[0].Position = outpts[0];
        polygons[2].v[1].Position = outpts[2];
        polygons[2].v[2].Position = outpts[3];
    }
    else if (in_c == 3 && out_c == 3)  // plane bisects the polygon
    {
        outputFlag = OneFrontOneBack;

        polygons[0].v[0].Position = inpts[0];
        polygons[0].v[1].Position = inpts[1];
        polygons[0].v[2].Position = inpts[2];
        polygons[1].v[0].Position = outpts[0];
        polygons[1].v[1].Position = outpts[1];
        polygons[1].v[2].Position = outpts[2];
    }
    else // then polygon must be totally infront of or behind the plane
    {
        int side;

        for (int loop = 0; loop < 3; loop++)
        {
            temp = polygonToSplit.v[loop].Position;
            side = ClassifyPoint(temp, pointOnPlane, planeNormal);
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

    // Normals
    glm::vec3 normal;
    for (bsp_tree::polygon& polygon : polygons)
    {
        normal = glm::normalize(glm::cross(polygon.v[1].Position - polygon.v[0].Position, polygon.v[2].Position - polygon.v[0].Position));
        polygon.v[0].Normal = normal;
        polygon.v[1].Normal = normal;
        polygon.v[2].Normal = normal;
    }

    return outputFlag;
}
