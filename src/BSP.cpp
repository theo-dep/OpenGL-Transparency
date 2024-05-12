// BSP functions    by Alan Baylis 2001
// https://www.alsprogrammingresource.com/bsp.html
// and https://github.com/GerardMT/BSP-Tree/tree/master

#include "BSP.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/vec_swizzle.hpp>

GLuint g_bspVboId = 0;
GLuint g_bspEboId = 0;
GLuint g_bspVaoId = 0;

bsp_tree::bsp_tree(const std::vector<Vertex> &vertices) : vertices_(vertices) {
}

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
    construct_rec(polygons, root_);

    glGenBuffers(1, &g_bspVboId);
    glGenBuffers(1, &g_bspEboId);
    glGenVertexArrays(1, &g_bspVaoId);

    glBindVertexArray(g_bspVaoId);

    CreateBufferData(g_bspVboId, g_bspEboId, vertices_, fragments() * sizeof(polygon), GL_STREAM_DRAW);
}

void bsp_tree::construct_rec(const std::vector<polygon> &polygons, node *n) {
    unsigned int pol_i = polygon_index(polygons);
    n->pols.push_back(polygons[pol_i]);

    to_plane(polygons[pol_i], n->plane);

    std::vector<polygon> polygons_front;
    std::vector<polygon> polygons_back;

    for (unsigned int i = 0; i < polygons.size(); ++i) {
        if (i != pol_i) {
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
        }
    }

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
    glm::vec3 u = vertices_[pol.i[1]].Position - vertices_[pol.i[0]].Position;
    glm::vec3 v = vertices_[pol.i[2]].Position - vertices_[pol.i[0]].Position;

    glm::vec3 r = glm::cross(u, v);
    pl.x = r.x;
    pl.y = r.y;
    pl.z = r.z;
    pl.w = -glm::dot(glm::xyz(pl), vertices_[pol.i[0]].Position);
}

bsp_tree::dist_res bsp_tree::distance(const glm::vec4 &pl, const polygon &pol) const {
    float d1 = glm::dot(pl, glm::vec4(vertices_[pol.i[0]].Position, 1));
    float d2 = glm::dot(pl, glm::vec4(vertices_[pol.i[1]].Position, 1));

    if (d1 < 0 && d2 > 0) {
        return bsp_tree::HALF;
    } else {
        float d3 = glm::dot(pl, glm::vec4(vertices_[pol.i[2]].Position, 1));

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

#define ADD_VERTEX(Poly, Point, Normal, Index) \
    vertices_.push_back({ Point, Normal }); Poly.i[Index] = vertices_.size() - 1;

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
    float d1 = glm::dot(pl, glm::vec4(vertices_[pol.i[0]].Position, 1));
    float d2 = glm::dot(pl, glm::vec4(vertices_[pol.i[1]].Position, 1));
    float d3 = glm::dot(pl, glm::vec4(vertices_[pol.i[2]].Position, 1));

    if (d1 < 0 && d2 > 0 && d3 > 0) {
        polygon_split_aux(pl, vertices_[pol.i[0]].Position, vertices_[pol.i[1]].Position, vertices_[pol.i[2]].Position, polygons_front, polygons_back);
    } else if (d2 < 0 && d1 > 0 && d3 > 0) {
        polygon_split_aux(pl, vertices_[pol.i[1]].Position, vertices_[pol.i[0]].Position, vertices_[pol.i[2]].Position, polygons_front, polygons_back);
    } else if (d3 < 0 && d1 > 0 && d2 > 0) {
        polygon_split_aux(pl, vertices_[pol.i[2]].Position, vertices_[pol.i[0]].Position, vertices_[pol.i[1]].Position, polygons_front, polygons_back);
    } else if (d1 > 0 && d2 < 0 && d3 < 0) {
        polygon_split_aux(pl, vertices_[pol.i[0]].Position, vertices_[pol.i[1]].Position, vertices_[pol.i[2]].Position, polygons_back, polygons_front);
    } else if (d2 > 0 && d1 < 0 && d3 < 0) {
        polygon_split_aux(pl, vertices_[pol.i[1]].Position, vertices_[pol.i[0]].Position, vertices_[pol.i[2]].Position, polygons_back, polygons_front);
    } else if (d3 > 0 && d1 < 0 && d2 < 0) {
        polygon_split_aux(pl, vertices_[pol.i[0]].Position, vertices_[pol.i[1]].Position, vertices_[pol.i[2]].Position, polygons_back, polygons_front);
    }
}

bsp_tree::~bsp_tree() {
    glDeleteBuffers(1, &g_bspVboId);
    glDeleteBuffers(1, &g_bspEboId);
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

inline int ClassifyPoint(const glm::vec3& point, const glm::vec3& pO, const glm::vec3& pN);

void bsp_tree::render(const glm::mat4& modelViewProjectionMatrix) {
    std::vector<polygon> polygons;
    render_rec(root_, modelViewProjectionMatrix, polygons);

    glBindVertexArray(g_bspVaoId);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_bspEboId);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, polygons.size() * sizeof(polygon), &polygons[0]);

    glDrawElements(GL_TRIANGLES, polygons.size(), GL_UNSIGNED_INT, 0);
}

void bsp_tree::render_rec(node *n, const glm::mat4& modelViewProjectionMatrix, std::vector<polygon> &polygons) {
    int side;
    glm::vec3 position, edge1, edge2, planeNormal, temp;

    //The current position of the player/viewpoint
    position.x = glm::column(modelViewProjectionMatrix, 3).x;
    position.y = glm::column(modelViewProjectionMatrix, 3).y;
    position.z = glm::column(modelViewProjectionMatrix, 3).z;

    if (n->l != nullptr || n->r != nullptr)
    {
        // get the partitioning planes normal
        planeNormal = glm::xyz(n->plane);
        temp.x = vertices_[n->pols[0].i[0]].Position.x;
        temp.y = vertices_[n->pols[0].i[0]].Position.y;
        temp.z = vertices_[n->pols[0].i[0]].Position.z;
        side = ClassifyPoint(position, temp, planeNormal);

        if (side == -1)
        {
            render_rec(n->r, modelViewProjectionMatrix, polygons);
            render_rec(n->l, modelViewProjectionMatrix, polygons);
        }
        else
        {
            render_rec(n->l, modelViewProjectionMatrix, polygons);
            render_rec(n->r, modelViewProjectionMatrix, polygons);
        }
    }

    if (n->l == nullptr && n->r == nullptr)
    {
        //Draw polygons that are in the leaf
        for (unsigned int loop = 0; loop < n->pols.size(); loop++)
        {
            polygons.push_back(n->pols[loop]);
        }
    }
}

inline int ClassifyPoint(const glm::vec3& point, const glm::vec3& pO, const glm::vec3& pN)
{
    glm::vec3 TempVect;
    TempVect.x = pO.x - point.x;
    TempVect.y = pO.y - point.y;
    TempVect.z = pO.z - point.z;
    glm::vec3 dir = TempVect;
    float d = glm::dot(dir, pN);

    if (d < -0.001f)
        return 1;
    else
        if (d > 0.001f)
            return -1;
        return 0;
}
