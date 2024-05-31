#include "Mesh.h"

//--------------------------------------------------------------------------
void CreateBufferData(GLuint vboId, GLuint eboId, const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
{
    glBindBuffer(GL_ARRAY_BUFFER, vboId);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLubyte*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLubyte*)offsetof(Vertex, Normal));
}

//--------------------------------------------------------------------------
unsigned int* CreateMappedBufferData(GLuint vboId, GLuint eboId, const std::vector<Vertex>& vertices, unsigned int indexSize)
{
    glBindBuffer(GL_ARRAY_BUFFER, vboId);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLubyte*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLubyte*)offsetof(Vertex, Normal));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboId);
    GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    GLsizeiptr bufferSize = indexSize * sizeof(unsigned int);
    glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, bufferSize, 0, flags);
    return (unsigned int*)glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, bufferSize, flags);
}
