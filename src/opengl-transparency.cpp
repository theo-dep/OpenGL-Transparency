//--------------------------------------------------------------------------------------
// Order Independent Transparency with Dual Depth Peeling
//
// Author: Louis Bavoil
// Email: sdkfeedback@nvidia.com
//
// Depth peeling is traditionally used to perform order independent transparency (OIT)
// with N geometry passes for N transparency layers. Dual depth peeling enables peeling
// N transparency layers in N/2+1 passes, by peeling from the front and the back
// simultaneously using a min-max depth buffer. This sample performs either normal or
// dual depth peeling and blends on the fly.
//
// Copyright (c) NVIDIA Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma warning( disable : 4996 )

#include "GLSLProgramObject.h"
#include "Mesh.h"
#include "OSD.h"

#include <GL/glew.h>
#include <GL/freeglut.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <algorithm>
#include <array>
#include <memory>
#include <chrono>
#include <unordered_map>
#include <filesystem>

#define FOVY 30.0f
#define ZNEAR 0.0001f
#define ZFAR 10.0f
#define FPS_TIME_WINDOW 1
#define MAX_DEPTH 1.0

int g_numPasses = 4;
int g_imageWidth = 1024;
int g_imageHeight = 768;

const aiScene *g_scene;
const aiMesh *g_model;
GLuint g_vboId, g_eboId, g_vaoId;
GLuint g_sortedVboId, g_sortedEboId, g_sortedVaoId;
unsigned int g_modelIndexCount;

bool g_useOQ = true;
GLuint g_queryId;

GLSLProgramObject g_shader3d;

GLSLProgramObject g_shaderDualInit;
GLSLProgramObject g_shaderDualPeel;
GLSLProgramObject g_shaderDualBlend;
GLSLProgramObject g_shaderDualFinal;

GLSLProgramObject g_shaderFrontInit;
GLSLProgramObject g_shaderFrontPeel;
GLSLProgramObject g_shaderFrontBlend;
GLSLProgramObject g_shaderFrontFinal;

GLSLProgramObject g_shaderAverageInit;
GLSLProgramObject g_shaderAverageFinal;

GLSLProgramObject g_shaderWeightedSumInit;
GLSLProgramObject g_shaderWeightedSumFinal;

float g_opacity = 0.6f;
char g_mode = DUAL_PEELING_MODE;
bool g_showOsd = true;
bool g_bShowUI = true;
unsigned g_numGeoPasses = 0;

int g_rotating = 0;
int g_panning = 0;
int g_scaling = 0;
int g_oldX, g_oldY;
int g_newX, g_newY;
float g_bbScale = 1.0;
glm::vec3 g_bbTrans(0.0, 0.0, 0.0);
glm::vec2 g_rot(15.0, 45.0);
glm::vec3 g_pos(0.0, 0.0, 1.0);

glm::mat4 g_projetionMatrix;
glm::mat4 g_modelViewMatrix;

glm::vec3 g_white(1);
glm::vec3 g_black(0);
glm::vec3 g_backgroundColor = g_white;

GLuint g_dualBackBlenderFboId;
GLuint g_dualPeelingSingleFboId;
GLuint g_dualDepthTexId[2];
GLuint g_dualFrontBlenderTexId[2];
GLuint g_dualBackTempTexId[2];
GLuint g_dualBackBlenderTexId;

GLuint g_frontFboId[2];
GLuint g_frontDepthTexId[2];
GLuint g_frontColorTexId[2];
GLuint g_frontColorBlenderTexId;
GLuint g_frontColorBlenderFboId;

GLuint g_accumulationTexId[2];
GLuint g_accumulationFboId;

GLenum g_drawBuffers[] = { GL_COLOR_ATTACHMENT0,
                           GL_COLOR_ATTACHMENT1,
                           GL_COLOR_ATTACHMENT2,
                           GL_COLOR_ATTACHMENT3,
                           GL_COLOR_ATTACHMENT4,
                           GL_COLOR_ATTACHMENT5,
                           GL_COLOR_ATTACHMENT6
};

//--------------------------------------------------------------------------
void InitDualPeelingRenderTargets()
{
    glGenTextures(2, g_dualDepthTexId);
    glGenTextures(2, g_dualFrontBlenderTexId);
    glGenTextures(2, g_dualBackTempTexId);
    glGenFramebuffers(1, &g_dualPeelingSingleFboId);
    for (int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_RECTANGLE, g_dualDepthTexId[i]);
        glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RG32F, g_imageWidth, g_imageHeight, 0, GL_RG, GL_FLOAT, 0);

        glBindTexture(GL_TEXTURE_RECTANGLE, g_dualFrontBlenderTexId[i]);
        glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA32F, g_imageWidth, g_imageHeight, 0, GL_RGBA, GL_FLOAT, 0);

        glBindTexture(GL_TEXTURE_RECTANGLE, g_dualBackTempTexId[i]);
        glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA32F, g_imageWidth, g_imageHeight, 0, GL_RGBA, GL_FLOAT, 0);
    }

    glGenTextures(1, &g_dualBackBlenderTexId);
    glBindTexture(GL_TEXTURE_RECTANGLE, g_dualBackBlenderTexId);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGB32F, g_imageWidth, g_imageHeight, 0, GL_RGB, GL_FLOAT, 0);

    glGenFramebuffers(1, &g_dualBackBlenderFboId);
    glBindFramebuffer(GL_FRAMEBUFFER, g_dualBackBlenderFboId);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, g_dualBackBlenderTexId, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, g_dualPeelingSingleFboId);

    int j = 0;
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, g_dualDepthTexId[j], 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_RECTANGLE, g_dualFrontBlenderTexId[j], 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_RECTANGLE, g_dualBackTempTexId[j], 0);

    j = 1;
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_RECTANGLE, g_dualDepthTexId[j], 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_RECTANGLE, g_dualFrontBlenderTexId[j], 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT5, GL_TEXTURE_RECTANGLE, g_dualBackTempTexId[j], 0);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT6, GL_TEXTURE_RECTANGLE, g_dualBackBlenderTexId, 0);

    CHECK_GL_ERRORS;
}

//--------------------------------------------------------------------------
void DeleteDualPeelingRenderTargets()
{
    glDeleteFramebuffers(1, &g_dualBackBlenderFboId);
    glDeleteFramebuffers(1, &g_dualPeelingSingleFboId);
    glDeleteTextures(2, g_dualDepthTexId);
    glDeleteTextures(2, g_dualFrontBlenderTexId);
    glDeleteTextures(2, g_dualBackTempTexId);
    glDeleteTextures(1, &g_dualBackBlenderTexId);
}

//--------------------------------------------------------------------------
void InitFrontPeelingRenderTargets()
{
    glGenTextures(2, g_frontDepthTexId);
    glGenTextures(2, g_frontColorTexId);
    glGenFramebuffers(2, g_frontFboId);

    for (int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_RECTANGLE, g_frontDepthTexId[i]);
        glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_DEPTH_COMPONENT32F, g_imageWidth, g_imageHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

        glBindTexture(GL_TEXTURE_RECTANGLE, g_frontColorTexId[i]);
        glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA32F, g_imageWidth, g_imageHeight, 0, GL_RGBA, GL_FLOAT, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, g_frontFboId[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_RECTANGLE, g_frontDepthTexId[i], 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, g_frontColorTexId[i], 0);
    }

    glGenTextures(1, &g_frontColorBlenderTexId);
    glBindTexture(GL_TEXTURE_RECTANGLE, g_frontColorBlenderTexId);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA32F, g_imageWidth, g_imageHeight, 0, GL_RGBA, GL_FLOAT, 0);

    glGenFramebuffers(1, &g_frontColorBlenderFboId);
    glBindFramebuffer(GL_FRAMEBUFFER, g_frontColorBlenderFboId);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_RECTANGLE, g_frontDepthTexId[0], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, g_frontColorBlenderTexId, 0);

    CHECK_GL_ERRORS;
}

//--------------------------------------------------------------------------
void DeleteFrontPeelingRenderTargets()
{
    glDeleteFramebuffers(2, g_frontFboId);
    glDeleteFramebuffers(1, &g_frontColorBlenderFboId);
    glDeleteTextures(2, g_frontDepthTexId);
    glDeleteTextures(2, g_frontColorTexId);
    glDeleteTextures(1, &g_frontColorBlenderTexId);
}

//--------------------------------------------------------------------------
void InitAccumulationRenderTargets()
{
    glGenTextures(2, g_accumulationTexId);

    glBindTexture(GL_TEXTURE_RECTANGLE, g_accumulationTexId[0]);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA16F, g_imageWidth, g_imageHeight, 0, GL_RGBA, GL_FLOAT, NULL);

    glBindTexture(GL_TEXTURE_RECTANGLE, g_accumulationTexId[1]);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_R32F, g_imageWidth, g_imageHeight, 0, GL_RGBA, GL_FLOAT, NULL);

    glGenFramebuffers(1, &g_accumulationFboId);
    glBindFramebuffer(GL_FRAMEBUFFER, g_accumulationFboId);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, g_accumulationTexId[0], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_RECTANGLE, g_accumulationTexId[1], 0);

    CHECK_GL_ERRORS;
}

//--------------------------------------------------------------------------
void DeleteAccumulationRenderTargets()
{
    glDeleteFramebuffers(1, &g_accumulationFboId);
    glDeleteTextures(2, g_accumulationTexId);

    CHECK_GL_ERRORS;
}

// Function to sort triangles and reorganize vertex data in ascending order
//--------------------------------------------------------------------------
void SortAndReorganizeTriangles(std::vector<unsigned int>& indices, std::vector<Vertex>& vertices) {
    if (indices.size() % 3 != 0) {
        std::cerr << "The list size must be a multiple of 3." << std::endl;
        return;
    }

    // Calculate average distances and sort triangles
    struct Triangle {
        float averageDistance;
        std::array<unsigned int, 3> indices;
    };

    std::vector<Triangle> triangles;
    for (size_t i = 0; i < indices.size(); i += 3) {
        glm::vec3 p1 = vertices[indices[i]].Position;
        glm::vec3 p2 = vertices[indices[i+1]].Position;
        glm::vec3 p3 = vertices[indices[i+2]].Position;
        float avgDist = (glm::length(p1) + glm::length(p2) + glm::length(p3)) / 3.0f;
        triangles.push_back({ avgDist, { indices[i], indices[i+1], indices[i+2] }});
    }

    // Sort triangles by average distance in ascending order
    std::sort(triangles.begin(), triangles.end(), [](const Triangle& a, const Triangle& b) {
        return a.averageDistance < b.averageDistance;
    });

    // Reorganize indices and vertices
    std::vector<Vertex> newVertices;
    std::vector<unsigned int> newIndices;
    std::unordered_map<unsigned int, unsigned int> indexMap;

    for (const auto& triangle : triangles) {
        for (int i = 0; i < 3; ++i) {
            unsigned int oldIndex = triangle.indices[i];
            if (indexMap.find(oldIndex) == indexMap.end()) {
                indexMap[oldIndex] = newVertices.size();
                newVertices.push_back(vertices[oldIndex]);
            }
            newIndices.push_back(indexMap[oldIndex]);
        }
    }

    // Replace original lists with sorted lists
    vertices = std::move(newVertices);
    indices = std::move(newIndices);
}

//--------------------------------------------------------------------------
void LoadModel()
{
    printf("loading OBJ...\n");
    const std::string model_filename = std::filesystem::canonical("models/dragon.obj").string();
    Assimp::Importer importer;
    g_scene = importer.ReadFile(model_filename,
        aiProcess_CalcTangentSpace       |
        aiProcess_Triangulate            |
        aiProcess_JoinIdenticalVertices  |
        aiProcess_SortByPType |
        aiProcess_GenBoundingBoxes);

    if (g_scene == nullptr || g_scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || g_scene->mNumMeshes != 1) {
        std::cerr << "Error loading model " << model_filename << std::endl;
        exit(1);
    }

    g_model = g_scene->mMeshes[0];

    if (!g_model->HasNormals()) {
        std::cerr << "Error model has no normals " << model_filename << std::endl;
        exit(1);
    }

    printf("%d vertices\n", g_model->mNumVertices);
    printf("%d triangles\n", g_model->mNumFaces);

    std::vector<Vertex> vertices;
    vertices.reserve(g_model->mNumVertices);

    for (unsigned int i = 0; i < g_model->mNumVertices; ++i) {
        Vertex vertex;
        glm::vec3 vector;
        vector.x = g_model->mVertices[i].x;
        vector.y = g_model->mVertices[i].y;
        vector.z = g_model->mVertices[i].z;
        vertex.Position = vector;

        vector.x = g_model->mNormals[i].x;
        vector.y = g_model->mNormals[i].y;
        vector.z = g_model->mNormals[i].z;
        vertex.Normal = vector;

        vertices.push_back(vertex);
    }

    std::vector<unsigned int> indices;
    indices.reserve(g_model->mNumFaces * 3);

    for (unsigned int i = 0; i < g_model->mNumFaces; ++i) {
        const aiFace &face = g_model->mFaces[i];
        for (unsigned int j = 0; j < 3 && j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    g_modelIndexCount = indices.size();

    glGenBuffers(1, &g_vboId);
    glGenBuffers(1, &g_eboId);
    glGenVertexArrays(1, &g_vaoId);

    glBindVertexArray(g_vaoId);

    CreateBufferData(g_vboId, g_eboId, vertices, indices);

    glGenBuffers(1, &g_sortedVboId);
    glGenBuffers(1, &g_sortedEboId);
    glGenVertexArrays(1, &g_sortedVaoId);

    glBindVertexArray(g_sortedVaoId);

    SortAndReorganizeTriangles(indices, vertices);
    CreateBufferData(g_sortedVboId, g_sortedEboId, vertices, indices);

    const aiAABB &aabb = g_model->mAABB;
    const glm::vec3 modelMin(aabb.mMin.x, aabb.mMin.y, aabb.mMin.z);
    const glm::vec3 modelMax(aabb.mMax.x, aabb.mMax.y, aabb.mMax.z);

    glm::vec3 diag = modelMax - modelMin;
    g_bbScale = 1.0f / glm::length(diag) * 1.5f;
    g_bbTrans = -g_bbScale * (modelMin + 0.5f * (modelMax - modelMin));

    CHECK_GL_ERRORS;
}

//--------------------------------------------------------------------------
void DeleteModel()
{
    glDeleteBuffers(1, &g_vboId);
    glDeleteBuffers(1, &g_eboId);
    glDeleteVertexArrays(1, &g_vaoId);

    glDeleteBuffers(1, &g_sortedVboId);
    glDeleteBuffers(1, &g_sortedEboId);
    glDeleteVertexArrays(1, &g_sortedVaoId);

}

//--------------------------------------------------------------------------
void DrawModel(bool sorted = false)
{
    glBindVertexArray(sorted ? g_sortedVaoId : g_vaoId);
    glDrawElements(GL_TRIANGLES, g_modelIndexCount, GL_UNSIGNED_INT, 0);

    g_numGeoPasses++;

    CHECK_GL_ERRORS;
}

//--------------------------------------------------------------------------
void BuildShaders()
{
    printf("\nloading shaders...\n");

    g_shader3d.attachVertexShader("shade_vertex.glsl");
    g_shader3d.attachVertexShader("3d_vertex.glsl");
    g_shader3d.attachFragmentShader("shade_fragment.glsl");
    g_shader3d.attachFragmentShader("3d_fragment.glsl");
    g_shader3d.link();

    g_shaderDualInit.attachVertexShader("dual_peeling_init_vertex.glsl");
    g_shaderDualInit.attachFragmentShader("dual_peeling_init_fragment.glsl");
    g_shaderDualInit.link();

    g_shaderDualPeel.attachVertexShader("shade_vertex.glsl");
    g_shaderDualPeel.attachVertexShader("dual_peeling_peel_vertex.glsl");
    g_shaderDualPeel.attachFragmentShader("shade_fragment.glsl");
    g_shaderDualPeel.attachFragmentShader("dual_peeling_peel_fragment.glsl");
    g_shaderDualPeel.link();

    g_shaderDualBlend.attachVertexShader("quad_vertex.glsl");
    g_shaderDualBlend.attachFragmentShader("dual_peeling_blend_fragment.glsl");
    g_shaderDualBlend.link();

    g_shaderDualFinal.attachVertexShader("quad_vertex.glsl");
    g_shaderDualFinal.attachFragmentShader("dual_peeling_final_fragment.glsl");
    g_shaderDualFinal.link();

    g_shaderFrontInit.attachVertexShader("shade_vertex.glsl");
    g_shaderFrontInit.attachVertexShader("front_peeling_init_vertex.glsl");
    g_shaderFrontInit.attachFragmentShader("shade_fragment.glsl");
    g_shaderFrontInit.attachFragmentShader("front_peeling_init_fragment.glsl");
    g_shaderFrontInit.link();

    g_shaderFrontPeel.attachVertexShader("shade_vertex.glsl");
    g_shaderFrontPeel.attachVertexShader("front_peeling_peel_vertex.glsl");
    g_shaderFrontPeel.attachFragmentShader("shade_fragment.glsl");
    g_shaderFrontPeel.attachFragmentShader("front_peeling_peel_fragment.glsl");
    g_shaderFrontPeel.link();

    g_shaderFrontBlend.attachVertexShader("quad_vertex.glsl");
    g_shaderFrontBlend.attachFragmentShader("front_peeling_blend_fragment.glsl");
    g_shaderFrontBlend.link();

    g_shaderFrontFinal.attachVertexShader("quad_vertex.glsl");
    g_shaderFrontFinal.attachFragmentShader("front_peeling_final_fragment.glsl");
    g_shaderFrontFinal.link();

    g_shaderAverageInit.attachVertexShader("shade_vertex.glsl");
    g_shaderAverageInit.attachVertexShader("wavg_init_vertex.glsl");
    g_shaderAverageInit.attachFragmentShader("shade_fragment.glsl");
    g_shaderAverageInit.attachFragmentShader("wavg_init_fragment.glsl");
    g_shaderAverageInit.link();

    g_shaderAverageFinal.attachVertexShader("quad_vertex.glsl");
    g_shaderAverageFinal.attachFragmentShader("wavg_final_fragment.glsl");
    g_shaderAverageFinal.link();

    g_shaderWeightedSumInit.attachVertexShader("shade_vertex.glsl");
    g_shaderWeightedSumInit.attachVertexShader("wsum_init_vertex.glsl");
    g_shaderWeightedSumInit.attachFragmentShader("shade_fragment.glsl");
    g_shaderWeightedSumInit.attachFragmentShader("wsum_init_fragment.glsl");
    g_shaderWeightedSumInit.link();

    g_shaderWeightedSumFinal.attachVertexShader("quad_vertex.glsl");
    g_shaderWeightedSumFinal.attachFragmentShader("wsum_final_fragment.glsl");
    g_shaderWeightedSumFinal.link();

    LoadShaderText();

    CHECK_GL_ERRORS;
}

//--------------------------------------------------------------------------
void DestroyShaders()
{
    g_shader3d.destroy();

    g_shaderDualInit.destroy();
    g_shaderDualPeel.destroy();
    g_shaderDualBlend.destroy();
    g_shaderDualFinal.destroy();

    g_shaderFrontInit.destroy();
    g_shaderFrontPeel.destroy();
    g_shaderFrontBlend.destroy();
    g_shaderFrontFinal.destroy();

    g_shaderAverageInit.destroy();
    g_shaderAverageFinal.destroy();

    g_shaderWeightedSumInit.destroy();
    g_shaderWeightedSumFinal.destroy();

    DestroyShaderText();

    CHECK_GL_ERRORS;
}

//--------------------------------------------------------------------------
void ReloadShaders()
{
    DestroyShaders();
    BuildShaders();
}

//--------------------------------------------------------------------------
void InitGL()
{
    // Allocate render targets first
    InitDualPeelingRenderTargets();
    InitFrontPeelingRenderTargets();
    InitAccumulationRenderTargets();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    BuildShaders();
    LoadModel();
    InitFullScreenQuad();

    InitText();

    glEnable(GL_MULTISAMPLE);
    glDisable(GL_CULL_FACE);

    glGenQueries(1, &g_queryId);

    CHECK_GL_ERRORS;
}

//--------------------------------------------------------------------------
void DeleteGL()
{
    DeleteDualPeelingRenderTargets();
    DeleteFrontPeelingRenderTargets();
    DeleteAccumulationRenderTargets();

    DestroyShaders();
    DeleteModel();
    DeleteFullScreenQuad();

    DeleteText();

    glDeleteQueries(1, &g_queryId);

    CHECK_GL_ERRORS;
}

//--------------------------------------------------------------------------
glm::mat3 normalMatrix(const glm::mat4 &mat)
{
    const glm::mat3 mat3(mat);
    const glm::mat3 normalMat = glm::inverseTranspose(mat3);
    return normalMat;
}

//--------------------------------------------------------------------------
void RenderNormalBlending()
{
    glClearColor(g_backgroundColor[0], g_backgroundColor[1], g_backgroundColor[2], 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    g_shader3d.bind();
    g_shader3d.setUniform("ModelViewProjectionMatrix", (g_projetionMatrix * g_modelViewMatrix));
    g_shader3d.setUniform("ModelViewMatrix", g_modelViewMatrix);
    g_shader3d.setUniform("NormalMatrix", normalMatrix(g_modelViewMatrix));
    g_shader3d.setUniform("Alpha", g_opacity);
    DrawModel(true);

    glDisable(GL_BLEND);

    CHECK_GL_ERRORS;
}

//--------------------------------------------------------------------------
void RenderDualPeeling()
{
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    // ---------------------------------------------------------------------
    // 1. Initialize Min-Max Depth Buffer
    // ---------------------------------------------------------------------

    glBindFramebuffer(GL_FRAMEBUFFER, g_dualPeelingSingleFboId);

    // Render targets 1 and 2 store the front and back colors
    // Clear to 0.0 and use MAX blending to filter written color
    // At most one front color and one back color can be written every pass
    glDrawBuffers(2, &g_drawBuffers[1]);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    // Render target 0 stores (-minDepth, maxDepth, alphaMultiplier)
    glDrawBuffer(g_drawBuffers[0]);
    glClearColor(-MAX_DEPTH, -MAX_DEPTH, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glBlendEquation(GL_MAX);

    g_shaderDualInit.bind();
    g_shaderDualInit.setUniform("ModelViewProjectionMatrix", (g_projetionMatrix * g_modelViewMatrix));
    DrawModel();

    CHECK_GL_ERRORS;

    // ---------------------------------------------------------------------
    // 2. Dual Depth Peeling + Blending
    // ---------------------------------------------------------------------

    // Since we cannot blend the back colors in the geometry passes,
    // we use another render target to do the alpha blending
    //glBindFramebuffer(GL_FRAMEBUFFER, g_dualBackBlenderFboId);
    glDrawBuffer(g_drawBuffers[6]);
    glClearColor(g_backgroundColor[0], g_backgroundColor[1], g_backgroundColor[2], 0);
    glClear(GL_COLOR_BUFFER_BIT);

    int currId = 0;

    for (int pass = 1; g_useOQ || pass < g_numPasses; pass++) {
        currId = pass % 2;
        int prevId = 1 - currId;
        int bufId = currId * 3;

        //glBindFramebuffer(GL_FRAMEBUFFER, g_dualPeelingFboId[currId]);

        glDrawBuffers(2, &g_drawBuffers[bufId+1]);
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);

        glDrawBuffer(g_drawBuffers[bufId+0]);
        glClearColor(-MAX_DEPTH, -MAX_DEPTH, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);

        // Render target 0: RG32F MAX blending
        // Render target 1: RGBA MAX blending
        // Render target 2: RGBA MAX blending
        glDrawBuffers(3, &g_drawBuffers[bufId+0]);
        glBlendEquation(GL_MAX);

        g_shaderDualPeel.bind();
        g_shaderDualPeel.setUniform("ModelViewProjectionMatrix", (g_projetionMatrix * g_modelViewMatrix));
        g_shaderDualPeel.setUniform("ModelViewMatrix", g_modelViewMatrix);
        g_shaderDualPeel.setUniform("NormalMatrix", normalMatrix(g_modelViewMatrix));
        g_shaderDualPeel.bindTextureRECT("DepthBlenderTex", g_dualDepthTexId[prevId], 0);
        g_shaderDualPeel.bindTextureRECT("FrontBlenderTex", g_dualFrontBlenderTexId[prevId], 1);
        g_shaderDualPeel.setUniform("Alpha", g_opacity);
        DrawModel();

        CHECK_GL_ERRORS;

        // Full screen pass to alpha-blend the back color
        glDrawBuffer(g_drawBuffers[6]);

        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        if (g_useOQ) {
            glBeginQuery(GL_SAMPLES_PASSED, g_queryId);
        }

        g_shaderDualBlend.bind();
        g_shaderDualBlend.bindTextureRECT("TempTex", g_dualBackTempTexId[currId], 0);
        DrawFullScreenQuad();

        CHECK_GL_ERRORS;

        if (g_useOQ) {
            glEndQuery(GL_SAMPLES_PASSED);
            GLuint sample_count;
            glGetQueryObjectuiv(g_queryId, GL_QUERY_RESULT, &sample_count);
            if (sample_count == 0) {
                break;
            }
        }
    }

    glDisable(GL_BLEND);

    // ---------------------------------------------------------------------
    // 3. Final Pass
    // ---------------------------------------------------------------------

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDrawBuffer(GL_BACK);

    g_shaderDualFinal.bind();
    //g_shaderDualFinal.bindTextureRECT("DepthBlenderTex", g_dualDepthTexId[currId], 0);
    g_shaderDualFinal.bindTextureRECT("FrontBlenderTex", g_dualFrontBlenderTexId[currId], 1);
    g_shaderDualFinal.bindTextureRECT("BackBlenderTex", g_dualBackBlenderTexId, 2);
    DrawFullScreenQuad();

    CHECK_GL_ERRORS;
}

//--------------------------------------------------------------------------
void RenderFrontToBackPeeling()
{
    // ---------------------------------------------------------------------
    // 1. Initialize Min Depth Buffer
    // ---------------------------------------------------------------------

    glBindFramebuffer(GL_FRAMEBUFFER, g_frontColorBlenderFboId);
    glDrawBuffer(g_drawBuffers[0]);

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    g_shaderFrontInit.bind();
    g_shaderFrontInit.setUniform("ModelViewProjectionMatrix", (g_projetionMatrix * g_modelViewMatrix));
    g_shaderFrontInit.setUniform("ModelViewMatrix", g_modelViewMatrix);
    g_shaderFrontInit.setUniform("NormalMatrix", normalMatrix(g_modelViewMatrix));
    g_shaderFrontInit.setUniform("Alpha", g_opacity);
    DrawModel();

    CHECK_GL_ERRORS;

    // ---------------------------------------------------------------------
    // 2. Depth Peeling + Blending
    // ---------------------------------------------------------------------

    int numLayers = (g_numPasses - 1) * 2;
    for (int layer = 1; g_useOQ || layer < numLayers; layer++) {
        int currId = layer % 2;
        int prevId = 1 - currId;

        glBindFramebuffer(GL_FRAMEBUFFER, g_frontFboId[currId]);
        glDrawBuffer(g_drawBuffers[0]);

        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);

        if (g_useOQ) {
            glBeginQuery(GL_SAMPLES_PASSED, g_queryId);
        }

        g_shaderFrontPeel.bind();
        g_shaderFrontPeel.setUniform("ModelViewProjectionMatrix", (g_projetionMatrix * g_modelViewMatrix));
        g_shaderFrontPeel.setUniform("ModelViewMatrix", g_modelViewMatrix);
        g_shaderFrontPeel.setUniform("NormalMatrix", normalMatrix(g_modelViewMatrix));
        g_shaderFrontPeel.bindTextureRECT("DepthTex", g_frontDepthTexId[prevId], 0);
        g_shaderFrontPeel.setUniform("Alpha", g_opacity);
        DrawModel();

        if (g_useOQ) {
            glEndQuery(GL_SAMPLES_PASSED);
        }

        CHECK_GL_ERRORS;

        glBindFramebuffer(GL_FRAMEBUFFER, g_frontColorBlenderFboId);
        glDrawBuffer(g_drawBuffers[0]);

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);

        glBlendEquation(GL_FUNC_ADD);
        glBlendFuncSeparate(GL_DST_ALPHA, GL_ONE, GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);

        g_shaderFrontBlend.bind();
        g_shaderFrontBlend.bindTextureRECT("TempTex", g_frontColorTexId[currId], 0);
        DrawFullScreenQuad();

        glDisable(GL_BLEND);

        CHECK_GL_ERRORS;

        if (g_useOQ) {
            GLuint sample_count;
            glGetQueryObjectuiv(g_queryId, GL_QUERY_RESULT, &sample_count);
            if (sample_count == 0) {
                break;
            }
        }
    }

    // ---------------------------------------------------------------------
    // 3. Final Pass
    // ---------------------------------------------------------------------

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDrawBuffer(GL_BACK);
    glDisable(GL_DEPTH_TEST);

    g_shaderFrontFinal.bind();
    g_shaderFrontFinal.setUniform("BackgroundColor", g_backgroundColor);
    g_shaderFrontFinal.bindTextureRECT("ColorTex", g_frontColorBlenderTexId, 0);
    DrawFullScreenQuad();

    CHECK_GL_ERRORS;
}

//--------------------------------------------------------------------------
void RenderAverageColors()
{
    glDisable(GL_DEPTH_TEST);

    // ---------------------------------------------------------------------
    // 1. Accumulate Colors and Depth Complexity
    // ---------------------------------------------------------------------

    glBindFramebuffer(GL_FRAMEBUFFER, g_accumulationFboId);
    glDrawBuffers(2, g_drawBuffers);

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);
    glEnable(GL_BLEND);

    g_shaderAverageInit.bind();
    g_shaderAverageInit.setUniform("ModelViewProjectionMatrix", (g_projetionMatrix * g_modelViewMatrix));
    g_shaderAverageInit.setUniform("ModelViewMatrix", g_modelViewMatrix);
    g_shaderAverageInit.setUniform("NormalMatrix", normalMatrix(g_modelViewMatrix));
    g_shaderAverageInit.setUniform("Alpha", g_opacity);
    DrawModel();

    glDisable(GL_BLEND);

    CHECK_GL_ERRORS;

    // ---------------------------------------------------------------------
    // 2. Approximate Blending
    // ---------------------------------------------------------------------

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDrawBuffer(GL_BACK);

    g_shaderAverageFinal.bind();
    g_shaderAverageFinal.setUniform("BackgroundColor", g_backgroundColor);
    g_shaderAverageFinal.bindTextureRECT("ColorTex0", g_accumulationTexId[0], 0);
    g_shaderAverageFinal.bindTextureRECT("ColorTex1", g_accumulationTexId[1], 1);
    DrawFullScreenQuad();

    CHECK_GL_ERRORS;
}

//--------------------------------------------------------------------------
void RenderWeightedSum()
{
    glDisable(GL_DEPTH_TEST);

    // ---------------------------------------------------------------------
    // 1. Accumulate (alpha * color) and (alpha)
    // ---------------------------------------------------------------------

    glBindFramebuffer(GL_FRAMEBUFFER, g_accumulationFboId);
    glDrawBuffer(g_drawBuffers[0]);

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);
    glEnable(GL_BLEND);

    g_shaderWeightedSumInit.bind();
    g_shaderWeightedSumInit.setUniform("ModelViewProjectionMatrix", (g_projetionMatrix * g_modelViewMatrix));
    g_shaderWeightedSumInit.setUniform("ModelViewMatrix", g_modelViewMatrix);
    g_shaderWeightedSumInit.setUniform("NormalMatrix", normalMatrix(g_modelViewMatrix));
    g_shaderWeightedSumInit.setUniform("Alpha", g_opacity);
    DrawModel();

    glDisable(GL_BLEND);

    CHECK_GL_ERRORS;

    // ---------------------------------------------------------------------
    // 2. Weighted Sum
    // ---------------------------------------------------------------------

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDrawBuffer(GL_BACK);

    g_shaderWeightedSumFinal.bind();
    g_shaderWeightedSumFinal.setUniform("BackgroundColor", g_backgroundColor);
    g_shaderWeightedSumFinal.bindTextureRECT("ColorTex", g_accumulationTexId[0], 0);
    DrawFullScreenQuad();

    CHECK_GL_ERRORS;
}

//--------------------------------------------------------------------------
void displayFunc()
{
    static std::chrono::steady_clock::time_point s_t0 = std::chrono::steady_clock::now();
    g_numGeoPasses = 0;

    g_modelViewMatrix = glm::lookAt(g_pos, glm::vec3(g_pos.x, g_pos.y, 0), glm::vec3(0, -1, 0));
    g_modelViewMatrix = glm::rotate(g_modelViewMatrix, glm::radians(g_rot.x), glm::vec3(1, 0, 0));
    g_modelViewMatrix = glm::rotate(g_modelViewMatrix, glm::radians(g_rot.y), glm::vec3(0, 1, 0));
    g_modelViewMatrix = glm::translate(g_modelViewMatrix, g_bbTrans);
    g_modelViewMatrix = glm::scale(g_modelViewMatrix, glm::vec3(g_bbScale));

    switch (g_mode) {
        case NORMAL_BLENDING_MODE:
            RenderNormalBlending();
            break;
        case DUAL_PEELING_MODE:
            RenderDualPeeling();
            break;
        case F2B_PEELING_MODE:
            RenderFrontToBackPeeling();
            break;
        case WEIGHTED_AVERAGE_MODE:
            RenderAverageColors();
            break;
        case WEIGHTED_SUM_MODE:
            RenderWeightedSum();
            break;
    }

    // ---------------------------------------------------------------------

    static unsigned s_frame = 0;
    s_frame++;

    static float s_fps = 0;
    std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
    std::chrono::seconds elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(t1 - s_t0);
    if (elapsedTime > std::chrono::seconds{ 1 }) {
        s_fps = static_cast<float>(s_frame / static_cast<double>(elapsedTime.count()));
        s_t0 = t1;
        s_frame = 0;
    }

    if (g_showOsd) {
        DrawOsd(g_mode, g_opacity, g_numGeoPasses, s_fps);
    }

    glutSwapBuffers();
}

//--------------------------------------------------------------------------
void reshapeFunc(int w, int h)
{
    if (g_imageWidth != w || g_imageHeight != h)
    {
        g_imageWidth = w;
        g_imageHeight = h;

        DeleteDualPeelingRenderTargets();
        InitDualPeelingRenderTargets();

        DeleteFrontPeelingRenderTargets();
        InitFrontPeelingRenderTargets();

        DeleteAccumulationRenderTargets();
        InitAccumulationRenderTargets();
    }

    g_projetionMatrix = glm::perspective(FOVY, (float)g_imageWidth / (float)g_imageHeight, ZNEAR, ZFAR);

    glViewport(0, 0, g_imageWidth, g_imageHeight);

    CHECK_GL_ERRORS;
}

//--------------------------------------------------------------------------
void idleFunc()
{
    glutPostRedisplay();
}

//--------------------------------------------------------------------------
void mouseFunc(int button, int state, int x, int y)
{
    g_newX = x; g_newY = y;

    if (button == GLUT_LEFT_BUTTON)
    {
        int mod = glutGetModifiers();
        if (mod == GLUT_ACTIVE_SHIFT) {
            g_scaling = !state;
        } else if (mod == GLUT_ACTIVE_CTRL) {
            g_panning = !state;
        } else {
            g_rotating = !state;
        }
    }
}

//--------------------------------------------------------------------------
void motionFunc(int x, int y)
{
    g_oldX = g_newX; g_oldY = g_newY;
    g_newX = x;      g_newY = y;

    float rel_x = (g_newX - g_oldX) / (float)g_imageWidth;
    float rel_y = (g_newY - g_oldY) / (float)g_imageHeight;
    if (g_rotating)
    {
        g_rot.y += (rel_x * 180);
        g_rot.x += (rel_y * 180);
    }
    else if (g_panning)
    {
        g_pos.x -= rel_x;
        g_pos.y += rel_y;
    }
    else if (g_scaling)
    {
        g_pos.z -= rel_y * g_pos.z;
    }

    glutPostRedisplay();
}

//--------------------------------------------------------------------------
void SaveFramebuffer()
{
    std::cout << "Writing image.ppm... " << std::flush;
    std::unique_ptr<float[]> pixels = std::make_unique<float[]>(3 * g_imageWidth * g_imageHeight);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, g_imageWidth, g_imageHeight, GL_RGB, GL_FLOAT, pixels.get());

    char filename[] = "image.ppm";
    std::ofstream fp(filename);
    fp << "P3\n" << g_imageWidth << " " << g_imageHeight << std::endl << "255\n";
    for (int i = g_imageHeight-1; i >=0; i--)
    {
        for (int j = 0; j < g_imageWidth; j++)
        {
            int pixelPos = (i*g_imageWidth+j)*3;
            int r = (int)(pixels[pixelPos+0]*255.0);
            int g = (int)(pixels[pixelPos+1]*255.0);
            int b = (int)(pixels[pixelPos+2]*255.0);
            fp << r << " " << g << " " << b << std::endl;
        }
    }
    fp.close();
    std::cout << "done!\n";
}

//--------------------------------------------------------------------------
void keyboardFunc(unsigned char key, int x, int y)
{
    key = (unsigned char)tolower(key);
    switch(key)
    {
        case 8:
            g_bShowUI = !g_bShowUI;
            break;
        case 'q':
            g_useOQ = !g_useOQ;
            break;
        case '+':
            g_numPasses++;
            break;
        case '-':
            g_numPasses--;
            break;
        case 'b':
            g_backgroundColor = (g_backgroundColor == g_white) ? g_black : g_white;
            break;
        case 'c':
            SaveFramebuffer();
            break;
        case 'o':
            g_showOsd = !g_showOsd;
            break;
        case 'r':
            ReloadShaders();
            break;
        case '0':
            g_mode = NORMAL_BLENDING_MODE;
            break;
        case '1':
            g_mode = DUAL_PEELING_MODE;
            break;
        case '2':
            g_mode = F2B_PEELING_MODE;
            break;
        case '3':
            g_mode = WEIGHTED_AVERAGE_MODE;
            break;
        case '4':
            g_mode = WEIGHTED_SUM_MODE;
            break;
        case 'a':
            g_opacity -= 0.05f;
            g_opacity = std::max(g_opacity, 0.0f);
            break;
        case 'd':
            g_opacity += 0.05f;
            g_opacity = std::min(g_opacity, 1.0f);
            break;
        case 27:
            exit(0);
            break;
    }
    glutPostRedisplay();
}

//--------------------------------------------------------------------------
void menuFunc(int i)
{
    keyboardFunc((unsigned char) i, 0, 0);
}

//--------------------------------------------------------------------------
void InitMenus()
{
    int objectMenuId = glutCreateMenu(menuFunc);
    {
        glutCreateMenu(menuFunc);
        glutAddMenuEntry("'0' - Normal blending mode", '0');
        glutAddMenuEntry("'1' - Dual peeling mode", '1');
        glutAddMenuEntry("'2' - Front peeling mode", '2');
        glutAddMenuEntry("'3' - Weighted average mode", '3');
        glutAddMenuEntry("'4' - Weighted sum mode", '4');
        glutAddMenuEntry("'A' - dec uniform opacity", 'A');
        glutAddMenuEntry("'D' - inc uniform opacity", 'D');
        glutAddMenuEntry("'R' - Reload shaders", 'R');
        glutAddMenuEntry("'B' - Change background color", 'B');
        glutAddMenuEntry("'Q' - Toggle occlusion queries", 'Q');
        glutAddMenuEntry("'-' - dec number of geometry passes", '-');
        glutAddMenuEntry("'+' - inc number of geometry passes", '+');
        glutAddMenuEntry("Quit (esc)", '\033');
        glutAttachMenu(GLUT_RIGHT_BUTTON);
    }
}

//--------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    std::filesystem::current_path(std::filesystem::canonical(argv[0]).parent_path());

    printf("dual_depth_peeling - sample comparing multiple order independent transparency techniques\n");
    printf("  Commands:\n");
    printf("     A/D       - Change uniform opacity\n");
    printf("     0         - Normal blending mode\n");
    printf("     1         - Dual peeling mode\n");
    printf("     2         - Front to back peeling mode\n");
    printf("     3         - Weighted average mode\n");
    printf("     4         - Weighted sum mode\n");
    printf("     R         - Reload all shaders\n");
    printf("     B         - Change background color\n");
    printf("     Q         - Toggle occlusion queries\n");
    printf("     +/-       - Change number of geometry passes\n\n");

    glutInit(&argc, argv);
    glutSetOption(GLUT_MULTISAMPLE, 8);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glutInitWindowSize(g_imageWidth, g_imageHeight);

    glutInitContextVersion(3, 3);
    glutInitContextProfile(GLUT_CORE_PROFILE);

    const int windowHandle = glutCreateWindow("Order Independent Transparency");
    if (windowHandle == -1)
    {
        printf("glutCreateWindow failed. Exiting...\n");
        exit(1);
    }

    if (glewInit() != GLEW_OK)
    {
        printf("glewInit failed. Exiting...\n");
        exit(1);
    }

    printf("GL version %s\n", glGetString(GL_VERSION));
    printf("GL vendor %s\n", glGetString(GL_VENDOR));
    printf("GL render %s\n", glGetString(GL_RENDERER));
    printf("GLSL version %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

    InitGL();
    InitMenus();
    glutDisplayFunc(displayFunc);
    glutReshapeFunc(reshapeFunc);
    glutIdleFunc(idleFunc);
    glutMouseFunc(mouseFunc);
    glutMotionFunc(motionFunc);
    glutKeyboardFunc(keyboardFunc);

    glutMainLoop();

    DeleteGL();

    return 0;
}
