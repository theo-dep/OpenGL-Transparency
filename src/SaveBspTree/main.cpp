// Utility to build a bsp tree into a file name
// argument: the Object file to build
// write a binary file of the same name then the obj file in the same location

#include "Mesh.h"
#include "VertexPartBspTree.h"

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
        std::cerr << "If the model is big, build it with parts like model-1.obj, model-2.obj..." << std::endl;
        return EXIT_FAILURE;
    }

    const std::string modelFilename{ argv[1] };
    std::cout << "Load and build BSP tree for " << modelFilename << std::endl;

    std::vector<std::string> filenameToLoad;

    { // detecting parts
        std::filesystem::path pathPart(modelFilename);
        const std::string fileExtension = pathPart.extension();
        const std::string filename = pathPart.filename().replace_extension("");
        std::size_t i = 1;
        pathPart.replace_filename(filename + "-" + std::to_string(i) + ".obj");
        if (std::filesystem::exists(pathPart))
        {
            do
            {
                std::cout << "Detecting part " << pathPart.string() << std::endl;
                filenameToLoad.push_back(pathPart.string());
                pathPart.replace_filename(filename + "-" + std::to_string(++i) + ".obj");
            } while (std::filesystem::exists(pathPart));
        }
        else
        {
            filenameToLoad.push_back(modelFilename);
        }
    }

    std::vector<std::shared_ptr<VertexPartBspTree>> bspTrees;
    for (const std::string& filename : filenameToLoad)
    {
        std::filesystem::path pathPart(filename);
        pathPart.replace_extension("bin");
        const std::string bspPartFilename{ pathPart.string() };

        std::shared_ptr<VertexPartBspTree> bspTree;
        if (std::filesystem::exists(pathPart))
        {
            std::cout << "Loading " << bspPartFilename << std::endl;

            bspTree = std::make_shared<VertexPartBspTree>();
            if (!bspTree->load(bspPartFilename))
            {
                std::cerr << "Error loading model " << filename << " to " << bspPartFilename << std::endl;
                return EXIT_FAILURE;
            }
        }
        else
        {
            std::cout << "Loading " << filename << std::endl;

            Assimp::Importer importer;
            const aiScene* scene = importer.ReadFile(filename,
                aiProcess_CalcTangentSpace       |
                aiProcess_Triangulate            |
                aiProcess_JoinIdenticalVertices  |
                aiProcess_SortByPType            |
                aiProcess_GenBoundingBoxes);

            if (scene == nullptr || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || scene->mNumMeshes != 1)
            {
                std::cerr << "Error loading model " << filename << std::endl;
                return EXIT_FAILURE;
            }

            const aiMesh* model = scene->mMeshes[0];

            if (!model->HasNormals())
            {
                std::cerr << "Error model has no normals " << filename << std::endl;
                return EXIT_FAILURE;
            }

            std::cout << model->mNumVertices << " vertices" << std::endl;
            std::cout << model->mNumFaces << " triangles" << std::endl;

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

            bspTree = std::make_shared<VertexPartBspTree>(std::move(vertices), indices);

            std::cout << "Saving " << bspPartFilename << std::endl;

            if (!bspTree->save(bspPartFilename))
            {
                std::cerr << "Error saving model " << filename << " to " << bspPartFilename << std::endl;
                return EXIT_FAILURE;
            }
        }

        const std::vector<Vertex>& bspVertices = bspTree->getVertices();
        std::vector<unsigned int> bspIndices = bspTree->sort(glm::vec3(-1, -1, -1));

        std::cout << bspVertices.size() << " vertices" << std::endl;
        std::cout << (bspIndices.size() / 3) << " triangles" << std::endl;

        bspTrees.push_back(bspTree);
    }

    std::filesystem::path path(modelFilename);
    path.replace_extension("bin");
    const std::string bspFilename{ path.string() };

    std::cout << "Saving " << bspFilename << std::endl;

    std::shared_ptr<VertexPartBspTree> bspTreeToSave;
    if (bspTrees.size() > 1)
    {
        std::cout << "Unifying parts" << std::endl;

        bspTreeToSave = std::accumulate(std::next(bspTrees.cbegin()), bspTrees.cend(), bspTrees.front(),
            [](const std::shared_ptr<VertexPartBspTree> & fullBspTree, const std::shared_ptr<VertexPartBspTree> & bspTree) -> std::shared_ptr<VertexPartBspTree>
            {
                return VertexPartBspTree::unify(*fullBspTree, *bspTree);
            });
    }
    else
    {
        bspTreeToSave = bspTrees.front();
    }

    if (!bspTreeToSave->save(bspFilename))
    {
        std::cerr << "Error saving model " << modelFilename << " to " << bspFilename << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
