#pragma once

#include "Mesh.h"

#include <glm/vec3.hpp>
#include <glm/geometric.hpp>

#include "thirdparty/bsptree.hpp"

typedef bsp::BspTree<std::vector<Vertex>, std::vector<unsigned int>> VertexBspTree;

namespace bsp
{
    template <>
    struct point_traits<glm::vec3>
    {
        typedef glm::vec3::value_type coordinate_type;

        static inline glm::vec3 cross(const glm::vec3& a, const glm::vec3& b)
        {
            return glm::normalize(glm::cross(a, b));
        }

        static inline coordinate_type dot(const glm::vec3& a, const glm::vec3& b)
        {
            return glm::dot(a, b);
        }
    };

    template <>
    struct vertex_traits<Vertex>
    {
        typedef glm::vec3 position_type;

        static inline const position_type& getPosition(const Vertex& v)
        {
            return v.Position;
        }

        static inline Vertex getInterpolatedVertex(const Vertex& a, const Vertex& b, float i)
        {
            return (a * (1 - i) + b * i);
        }
    };
}
