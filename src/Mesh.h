#pragma once

#include <glm/vec3.hpp>

#include <GL/glew.h>

#include <vector>

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
};

void CreateBufferData(GLuint vboId, GLuint eboId, const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
void CreateBufferData(GLuint vboId, GLuint eboId, const std::vector<Vertex>& vertices, unsigned int indexSize,  GLenum iboUsage);
