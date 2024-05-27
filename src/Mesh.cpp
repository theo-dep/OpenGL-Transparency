#include "Mesh.h"

//--------------------------------------------------------------------------
void CreateBufferData(GLuint vboId, GLuint eboId, const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
{
    CreateBufferData(vboId, eboId, vertices, indices.size() * sizeof(unsigned int), GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(unsigned int), &indices[0]);
}

//--------------------------------------------------------------------------
void CreateBufferData(GLuint vboId, GLuint eboId, const std::vector<Vertex>& vertices, unsigned int indexSize, GLenum iboUsage)
{
    glBindBuffer(GL_ARRAY_BUFFER, vboId);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexSize, nullptr, iboUsage);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLubyte*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLubyte*)offsetof(Vertex, Normal));
}
