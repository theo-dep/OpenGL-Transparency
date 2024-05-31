#pragma once

#include <glm/vec3.hpp>
#include <glm/geometric.hpp>

#ifndef NO_OPENGL
#include <GL/glew.h>
#endif

#include <vector>

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
};

#ifndef NO_OPENGL
void CreateBufferData(GLuint vboId, GLuint eboId, const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
unsigned int* CreateMappedBufferData(GLuint vboId, GLuint eboId, const std::vector<Vertex>& vertices, unsigned int indexSize);
#endif

inline Vertex operator*(const Vertex& v, float f)
{
    return { v.Position * f, v.Normal * f };
}

inline Vertex operator+(const Vertex& a, const Vertex& b)
{
    return { a.Position + b.Position, glm::normalize(a.Normal + b.Normal) };
}
