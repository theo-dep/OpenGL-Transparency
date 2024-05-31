// Utility to build a bsp tree into a file name
// argument: the Object file to build
// write a binary file of the same name then the obj file in the same location

#include "Mesh.h"
#include "VertexBspTree.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <filesystem>
#include <iostream>

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage:" << std::endl;
        std::cerr << "build-save-bsp-tree model.obj" << std::endl;
        return EXIT_FAILURE;
    }

    const std::string modelFilename{ argv[1] };
    std::cout << "Load and build BSP tree for " << modelFilename << std::endl;

    std::cout << "Loading OBJ..." << std::endl;

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(modelFilename,
        aiProcess_CalcTangentSpace       |
        aiProcess_Triangulate            |
        aiProcess_JoinIdenticalVertices  |
        aiProcess_SortByPType |
        aiProcess_GenBoundingBoxes);

    if (scene == nullptr || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || scene->mNumMeshes != 1)
    {
        std::cerr << "Error loading model " << modelFilename << std::endl;
        return EXIT_FAILURE;
    }

    const aiMesh* model = scene->mMeshes[0];

    if (!model->HasNormals())
    {
        std::cerr << "Error model has no normals " << modelFilename << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << std::to_string(model->mNumVertices) << " vertices" << std::endl;
    std::cout << std::to_string(model->mNumFaces) << " triangles" << std::endl;

    std::cout << "Building BSP..." << std::endl;

    std::vector<Vertex> vertices;
    vertices.reserve(model->mNumVertices);

    for (unsigned int i = 0; i < model->mNumVertices; ++i)
    {
        Vertex vertex;
        glm::vec3 vector;
        vector.x = model->mVertices[i].x;
        vector.y = model->mVertices[i].y;
        vector.z = model->mVertices[i].z;
        vertex.Position = vector;

        vector.x = model->mNormals[i].x;
        vector.y = model->mNormals[i].y;
        vector.z = model->mNormals[i].z;
        vertex.Normal = vector;

        vertices.push_back(vertex);
    }

    std::vector<unsigned int> indices;
    indices.reserve(model->mNumFaces * 3);

    for (unsigned int i = 0; i < model->mNumFaces; ++i)
    {
        const aiFace &face = model->mFaces[i];
        for (unsigned int j = 0; j < 3 && j < face.mNumIndices; j++)
        {
            indices.push_back(face.mIndices[j]);
        }
    }

    VertexBspTree bspTree(std::move(vertices), indices);

    const std::vector<Vertex>& bspVertices = bspTree.getVertices();
    std::vector<unsigned int> bspIndices = bspTree.sort(glm::vec3(-1, -1, -1));

    std::cout << std::to_string(bspVertices.size()) << " vertices" << std::endl;
    std::cout << std::to_string(bspIndices.size() / 3) << " triangles" << std::endl;


    std::filesystem::path path(modelFilename);
    path.replace_extension("bin");
    const std::string bspFilename{ path.string() };

    std::cout << "Write " << bspFilename << std::endl;
    if (!bspTree.save(bspFilename))
    {
        std::cerr << "Error loading model " << modelFilename << " to " << bspFilename << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
