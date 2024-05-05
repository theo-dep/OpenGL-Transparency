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

#include "nvModel.h"
#include <nvShaderUtils.h>
#include <nvSDKPath.h>
#include "GLSLProgramObject.h"
#include "Timer.h"
#include "OSD.h"

#ifdef __APPLE__
#include <OpenGL/GL.h>
#include <GLUT/glut.h>
#else
#include <GL/glew.h>
#include <GL/freeglut.h>
#endif

#include <glm/glm.hpp>

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <algorithm>
#include <array>
#include <time.h>

#define FOVY 30.0
#define ZNEAR 0.0001
#define ZFAR 10.0
#define FPS_TIME_WINDOW 1
#define MAX_DEPTH 1.0

int g_numPasses = 4;
int g_imageWidth = 1024;
int g_imageHeight = 768;

nv::Model *g_model;
GLuint g_vboId;
GLuint g_eboId;
GLuint g_quadDisplayList;

bool g_useOQ = true;
GLuint g_queryId;

#define MODEL_FILENAME "media/models/dragon.obj"
#define SHADER_PATH "src/shaders/"

static nv::SDKPath sdkPath;

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
glm::vec2 g_rot(0.0, 45.0);
glm::vec3 g_pos(0.0, 0.0, 2.0);

float g_white[3] = {1.0,1.0,1.0};
float g_black[3] = {0.0};
float *g_backgroundColor = g_white;

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

GLenum g_drawBuffers[] = { GL_COLOR_ATTACHMENT0_EXT,
                           GL_COLOR_ATTACHMENT1_EXT,
                           GL_COLOR_ATTACHMENT2_EXT,
                           GL_COLOR_ATTACHMENT3_EXT,
                           GL_COLOR_ATTACHMENT4_EXT,
                           GL_COLOR_ATTACHMENT5_EXT,
                           GL_COLOR_ATTACHMENT6_EXT
};

#if 0
#define CHECK_GL_ERRORS  \
{ \
    GLenum err = glGetError(); \
    if (err) \
        printf( "Error %x at line %d\n", err, __LINE__); \
}
#else
#define CHECK_GL_ERRORS {}
#endif

void keyboardFunc(unsigned char key, int x, int y);

//--------------------------------------------------------------------------
void InitDualPeelingRenderTargets()
{
    glGenTextures(2, g_dualDepthTexId);
    glGenTextures(2, g_dualFrontBlenderTexId);
    glGenTextures(2, g_dualBackTempTexId);
    glGenFramebuffersEXT(1, &g_dualPeelingSingleFboId);
    for (int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, g_dualDepthTexId[i]);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RG32F,
                     g_imageWidth, g_imageHeight, 0, GL_RGB, GL_FLOAT, 0);

        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, g_dualFrontBlenderTexId[i]);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA,
                     g_imageWidth, g_imageHeight, 0, GL_RGBA, GL_FLOAT, 0);

        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, g_dualBackTempTexId[i]);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA,
                     g_imageWidth, g_imageHeight, 0, GL_RGBA, GL_FLOAT, 0);
    }

    glGenTextures(1, &g_dualBackBlenderTexId);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, g_dualBackBlenderTexId);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB,
                 g_imageWidth, g_imageHeight, 0, GL_RGB, GL_FLOAT, 0);

    glGenFramebuffersEXT(1, &g_dualBackBlenderFboId);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, g_dualBackBlenderFboId);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                              GL_TEXTURE_RECTANGLE_ARB, g_dualBackBlenderTexId, 0);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, g_dualPeelingSingleFboId);

    int j = 0;
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                                  GL_TEXTURE_RECTANGLE_ARB, g_dualDepthTexId[j], 0);
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT,
                                  GL_TEXTURE_RECTANGLE_ARB, g_dualFrontBlenderTexId[j], 0);
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT2_EXT,
                                  GL_TEXTURE_RECTANGLE_ARB, g_dualBackTempTexId[j], 0);

    j = 1;
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT3_EXT,
                                  GL_TEXTURE_RECTANGLE_ARB, g_dualDepthTexId[j], 0);
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT4_EXT,
                                  GL_TEXTURE_RECTANGLE_ARB, g_dualFrontBlenderTexId[j], 0);
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT5_EXT,
                                  GL_TEXTURE_RECTANGLE_ARB, g_dualBackTempTexId[j], 0);

    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT6_EXT,
                              GL_TEXTURE_RECTANGLE_ARB, g_dualBackBlenderTexId, 0);

    CHECK_GL_ERRORS;
}

//--------------------------------------------------------------------------
void DeleteDualPeelingRenderTargets()
{
    glDeleteFramebuffersEXT(1, &g_dualBackBlenderFboId);
    glDeleteFramebuffersEXT(1, &g_dualPeelingSingleFboId);
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
    glGenFramebuffersEXT(2, g_frontFboId);

    for (int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, g_frontDepthTexId[i]);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_DEPTH_COMPONENT,
                     g_imageWidth, g_imageHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, g_frontColorTexId[i]);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA,
                     g_imageWidth, g_imageHeight, 0, GL_RGBA, GL_FLOAT, 0);

        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, g_frontFboId[i]);
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                                  GL_TEXTURE_RECTANGLE_ARB, g_frontDepthTexId[i], 0);
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                                  GL_TEXTURE_RECTANGLE_ARB, g_frontColorTexId[i], 0);
    }

    glGenTextures(1, &g_frontColorBlenderTexId);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, g_frontColorBlenderTexId);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA,
                 g_imageWidth, g_imageHeight, 0, GL_RGBA, GL_FLOAT, 0);

    glGenFramebuffersEXT(1, &g_frontColorBlenderFboId);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, g_frontColorBlenderFboId);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                              GL_TEXTURE_RECTANGLE_ARB, g_frontDepthTexId[0], 0);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                              GL_TEXTURE_RECTANGLE_ARB, g_frontColorBlenderTexId, 0);
    CHECK_GL_ERRORS;
}

//--------------------------------------------------------------------------
void DeleteFrontPeelingRenderTargets()
{
    glDeleteFramebuffersEXT(2, g_frontFboId);
    glDeleteFramebuffersEXT(1, &g_frontColorBlenderFboId);
    glDeleteTextures(2, g_frontDepthTexId);
    glDeleteTextures(2, g_frontColorTexId);
    glDeleteTextures(1, &g_frontColorBlenderTexId);
}

//--------------------------------------------------------------------------
void InitAccumulationRenderTargets()
{
    glGenTextures(2, g_accumulationTexId);

    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, g_accumulationTexId[0]);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA16F_ARB,
                 g_imageWidth, g_imageHeight, 0, GL_RGBA, GL_FLOAT, NULL);

    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, g_accumulationTexId[1]);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_R32F,
                 g_imageWidth, g_imageHeight, 0, GL_RGBA, GL_FLOAT, NULL);

    glGenFramebuffersEXT(1, &g_accumulationFboId);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, g_accumulationFboId);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                              GL_TEXTURE_RECTANGLE_ARB, g_accumulationTexId[0], 0);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT,
                              GL_TEXTURE_RECTANGLE_ARB, g_accumulationTexId[1], 0);

    CHECK_GL_ERRORS;
}

//--------------------------------------------------------------------------
void DeleteAccumulationRenderTargets()
{
    glDeleteFramebuffersEXT(1, &g_accumulationFboId);
    glDeleteTextures(2, g_accumulationTexId);
}

//--------------------------------------------------------------------------
void MakeFullScreenQuad()
{
    g_quadDisplayList = glGenLists(1);
    glNewList(g_quadDisplayList, GL_COMPILE);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0.0, 1.0, 0.0, 1.0);
    glBegin(GL_QUADS);
    {
        glVertex2f(0.0, 0.0);
        glVertex2f(1.0, 0.0);
        glVertex2f(1.0, 1.0);
        glVertex2f(0.0, 1.0);
    }
    glEnd();
    glPopMatrix();

    glEndList();
}

//--------------------------------------------------------------------------
void LoadModel(const char *model_filename)
{
    g_model = new nv::Model;
    printf("loading OBJ...\n");

    std::string resolved_path;

    if (sdkPath.getFilePath( model_filename, resolved_path)) {
        if (!g_model->loadModelFromFile(resolved_path.c_str())) {
            fprintf(stderr, "Error loading model '%s'\n", model_filename);
            exit(1);
        }
    }
    else {
        fprintf(stderr, "Failed to find model '%s'\n", model_filename);
        exit(1);
    }

    printf("compiling mesh...\n");
    g_model->compileModel();

    printf("%d vertices\n", g_model->getPositionCount());
    printf("%d triangles\n", g_model->getIndexCount()/3);
    int totalVertexSize = g_model->getCompiledVertexCount() * g_model->getCompiledVertexSize() * sizeof(GLfloat);
    int totalIndexSize = g_model->getCompiledIndexCount() * sizeof(GLuint);

    glGenBuffers(1, &g_vboId);
    glBindBuffer(GL_ARRAY_BUFFER, g_vboId);
    glBufferData(GL_ARRAY_BUFFER, totalVertexSize, g_model->getCompiledVertices(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &g_eboId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_eboId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, totalIndexSize, g_model->getCompiledIndices(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glm::vec3 modelMin, modelMax;
    g_model->computeBoundingBox(modelMin, modelMax);

    glm::vec3 diag = modelMax - modelMin;
    g_bbScale = 1.0f / glm::length(diag) * 1.5f;
    g_bbTrans = -g_bbScale * (modelMin + 0.5f * (modelMax - modelMin));
}

//--------------------------------------------------------------------------
void DrawModel()
{
    glBindBuffer(GL_ARRAY_BUFFER, g_vboId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_eboId);
    int stride = g_model->getCompiledVertexSize() * sizeof(GLfloat);
    int normalOffset = g_model->getCompiledNormalOffset() * sizeof(GLfloat);
    glVertexPointer(g_model->getPositionSize(), GL_FLOAT, stride, NULL);
    glNormalPointer(GL_FLOAT, stride, (GLubyte *)NULL + normalOffset);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);

    glDrawElements(GL_TRIANGLES, g_model->getCompiledIndexCount(), GL_UNSIGNED_INT, NULL);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);

    g_numGeoPasses++;
}

//--------------------------------------------------------------------------
void BuildShaders()
{
    printf("\nloading shaders...\n");

    g_shaderDualInit.attachVertexShader(SHADER_PATH "dual_peeling_init_vertex.glsl");
    g_shaderDualInit.attachFragmentShader(SHADER_PATH "dual_peeling_init_fragment.glsl");
    g_shaderDualInit.link();

    g_shaderDualPeel.attachVertexShader(SHADER_PATH "shade_vertex.glsl");
    g_shaderDualPeel.attachVertexShader(SHADER_PATH "dual_peeling_peel_vertex.glsl");
    g_shaderDualPeel.attachFragmentShader(SHADER_PATH "shade_fragment.glsl");
    g_shaderDualPeel.attachFragmentShader(SHADER_PATH "dual_peeling_peel_fragment.glsl");
    g_shaderDualPeel.link();

    g_shaderDualBlend.attachVertexShader(SHADER_PATH "dual_peeling_blend_vertex.glsl");
    g_shaderDualBlend.attachFragmentShader(SHADER_PATH "dual_peeling_blend_fragment.glsl");
    g_shaderDualBlend.link();

    g_shaderDualFinal.attachVertexShader(SHADER_PATH "dual_peeling_final_vertex.glsl");
    g_shaderDualFinal.attachFragmentShader(SHADER_PATH "dual_peeling_final_fragment.glsl");
    g_shaderDualFinal.link();

    g_shaderFrontInit.attachVertexShader(SHADER_PATH "shade_vertex.glsl");
    g_shaderFrontInit.attachVertexShader(SHADER_PATH "front_peeling_init_vertex.glsl");
    g_shaderFrontInit.attachFragmentShader(SHADER_PATH "shade_fragment.glsl");
    g_shaderFrontInit.attachFragmentShader(SHADER_PATH "front_peeling_init_fragment.glsl");
    g_shaderFrontInit.link();

    g_shaderFrontPeel.attachVertexShader(SHADER_PATH "shade_vertex.glsl");
    g_shaderFrontPeel.attachVertexShader(SHADER_PATH "front_peeling_peel_vertex.glsl");
    g_shaderFrontPeel.attachFragmentShader(SHADER_PATH "shade_fragment.glsl");
    g_shaderFrontPeel.attachFragmentShader(SHADER_PATH "front_peeling_peel_fragment.glsl");
    g_shaderFrontPeel.link();

    g_shaderFrontBlend.attachVertexShader(SHADER_PATH "front_peeling_blend_vertex.glsl");
    g_shaderFrontBlend.attachFragmentShader(SHADER_PATH "front_peeling_blend_fragment.glsl");
    g_shaderFrontBlend.link();

    g_shaderFrontFinal.attachVertexShader(SHADER_PATH "front_peeling_final_vertex.glsl");
    g_shaderFrontFinal.attachFragmentShader(SHADER_PATH "front_peeling_final_fragment.glsl");
    g_shaderFrontFinal.link();

    g_shaderAverageInit.attachVertexShader(SHADER_PATH "shade_vertex.glsl");
    g_shaderAverageInit.attachVertexShader(SHADER_PATH "wavg_init_vertex.glsl");
    g_shaderAverageInit.attachFragmentShader(SHADER_PATH "shade_fragment.glsl");
    g_shaderAverageInit.attachFragmentShader(SHADER_PATH "wavg_init_fragment.glsl");
    g_shaderAverageInit.link();

    g_shaderAverageFinal.attachVertexShader(SHADER_PATH "wavg_final_vertex.glsl");
    g_shaderAverageFinal.attachFragmentShader(SHADER_PATH "wavg_final_fragment.glsl");
    g_shaderAverageFinal.link();

    g_shaderWeightedSumInit.attachVertexShader(SHADER_PATH "shade_vertex.glsl");
    g_shaderWeightedSumInit.attachVertexShader(SHADER_PATH "wsum_init_vertex.glsl");
    g_shaderWeightedSumInit.attachFragmentShader(SHADER_PATH "shade_fragment.glsl");
    g_shaderWeightedSumInit.attachFragmentShader(SHADER_PATH "wsum_init_fragment.glsl");
    g_shaderWeightedSumInit.link();

    g_shaderWeightedSumFinal.attachVertexShader(SHADER_PATH "wsum_final_vertex.glsl");
    g_shaderWeightedSumFinal.attachFragmentShader(SHADER_PATH "wsum_final_fragment.glsl");
    g_shaderWeightedSumFinal.link();
}

//--------------------------------------------------------------------------
void DestroyShaders()
{
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
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

    BuildShaders();
    LoadModel(MODEL_FILENAME);
    MakeFullScreenQuad();

    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glDisable(GL_NORMALIZE);

    glGenQueries(1, &g_queryId);
}

//--------------------------------------------------------------------------
void RenderDualPeeling()
{
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    // ---------------------------------------------------------------------
    // 1. Initialize Min-Max Depth Buffer
    // ---------------------------------------------------------------------

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, g_dualPeelingSingleFboId);

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
    glBlendEquationEXT(GL_MAX_EXT);

    g_shaderDualInit.bind();
    DrawModel();
    g_shaderDualInit.unbind();

    CHECK_GL_ERRORS;

    // ---------------------------------------------------------------------
    // 2. Dual Depth Peeling + Blending
    // ---------------------------------------------------------------------

    // Since we cannot blend the back colors in the geometry passes,
    // we use another render target to do the alpha blending
    //glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, g_dualBackBlenderFboId);
    glDrawBuffer(g_drawBuffers[6]);
    glClearColor(g_backgroundColor[0], g_backgroundColor[1], g_backgroundColor[2], 0);
    glClear(GL_COLOR_BUFFER_BIT);

    int currId = 0;

    for (int pass = 1; g_useOQ || pass < g_numPasses; pass++) {
        currId = pass % 2;
        int prevId = 1 - currId;
        int bufId = currId * 3;

        //glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, g_dualPeelingFboId[currId]);

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
        glBlendEquationEXT(GL_MAX_EXT);

        g_shaderDualPeel.bind();
        g_shaderDualPeel.bindTextureRECT("DepthBlenderTex", g_dualDepthTexId[prevId], 0);
        g_shaderDualPeel.bindTextureRECT("FrontBlenderTex", g_dualFrontBlenderTexId[prevId], 1);
        g_shaderDualPeel.setUniform("Alpha", (float*)&g_opacity, 1);
        DrawModel();
        g_shaderDualPeel.unbind();

        CHECK_GL_ERRORS;

        // Full screen pass to alpha-blend the back color
        glDrawBuffer(g_drawBuffers[6]);

        glBlendEquationEXT(GL_FUNC_ADD);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        if (g_useOQ) {
            glBeginQuery(GL_SAMPLES_PASSED_ARB, g_queryId);
        }

        g_shaderDualBlend.bind();
        g_shaderDualBlend.bindTextureRECT("TempTex", g_dualBackTempTexId[currId], 0);
        glCallList(g_quadDisplayList);
        g_shaderDualBlend.unbind();

        CHECK_GL_ERRORS;

        if (g_useOQ) {
            glEndQuery(GL_SAMPLES_PASSED_ARB);
            GLuint sample_count;
            glGetQueryObjectuiv(g_queryId, GL_QUERY_RESULT_ARB, &sample_count);
            if (sample_count == 0) {
                break;
            }
        }
    }

    glDisable(GL_BLEND);

    // ---------------------------------------------------------------------
    // 3. Final Pass
    // ---------------------------------------------------------------------

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    glDrawBuffer(GL_BACK);

    g_shaderDualFinal.bind();
    g_shaderDualFinal.bindTextureRECT("DepthBlenderTex", g_dualDepthTexId[currId], 0);
    g_shaderDualFinal.bindTextureRECT("FrontBlenderTex", g_dualFrontBlenderTexId[currId], 1);
    g_shaderDualFinal.bindTextureRECT("BackBlenderTex", g_dualBackBlenderTexId, 2);
    glCallList(g_quadDisplayList);
    g_shaderDualFinal.unbind();

    CHECK_GL_ERRORS;
}

//--------------------------------------------------------------------------
void RenderFrontToBackPeeling()
{
    // ---------------------------------------------------------------------
    // 1. Initialize Min Depth Buffer
    // ---------------------------------------------------------------------

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, g_frontColorBlenderFboId);
    glDrawBuffer(g_drawBuffers[0]);

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    g_shaderFrontInit.bind();
    g_shaderFrontInit.setUniform("Alpha", (float*)&g_opacity, 1);
    DrawModel();
    g_shaderFrontInit.unbind();

    CHECK_GL_ERRORS;

    // ---------------------------------------------------------------------
    // 2. Depth Peeling + Blending
    // ---------------------------------------------------------------------

    int numLayers = (g_numPasses - 1) * 2;
    for (int layer = 1; g_useOQ || layer < numLayers; layer++) {
        int currId = layer % 2;
        int prevId = 1 - currId;

        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, g_frontFboId[currId]);
        glDrawBuffer(g_drawBuffers[0]);

        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);

        if (g_useOQ) {
            glBeginQuery(GL_SAMPLES_PASSED_ARB, g_queryId);
        }

        g_shaderFrontPeel.bind();
        g_shaderFrontPeel.bindTextureRECT("DepthTex", g_frontDepthTexId[prevId], 0);
        g_shaderFrontPeel.setUniform("Alpha", (float*)&g_opacity, 1);
        DrawModel();
        g_shaderFrontPeel.unbind();

        if (g_useOQ) {
            glEndQuery(GL_SAMPLES_PASSED_ARB);
        }

        CHECK_GL_ERRORS;

        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, g_frontColorBlenderFboId);
        glDrawBuffer(g_drawBuffers[0]);

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);

        glBlendEquation(GL_FUNC_ADD);
        glBlendFuncSeparate(GL_DST_ALPHA, GL_ONE,
                            GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);

        g_shaderFrontBlend.bind();
        g_shaderFrontBlend.bindTextureRECT("TempTex", g_frontColorTexId[currId], 0);
        glCallList(g_quadDisplayList);
        g_shaderFrontBlend.unbind();

        glDisable(GL_BLEND);

        CHECK_GL_ERRORS;

        if (g_useOQ) {
            GLuint sample_count;
            glGetQueryObjectuiv(g_queryId, GL_QUERY_RESULT_ARB, &sample_count);
            if (sample_count == 0) {
                break;
            }
        }
    }

    // ---------------------------------------------------------------------
    // 3. Final Pass
    // ---------------------------------------------------------------------

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    glDrawBuffer(GL_BACK);
    glDisable(GL_DEPTH_TEST);

    g_shaderFrontFinal.bind();
    g_shaderFrontFinal.setUniform("BackgroundColor", g_backgroundColor, 3);
    g_shaderFrontFinal.bindTextureRECT("ColorTex", g_frontColorBlenderTexId, 0);
    glCallList(g_quadDisplayList);
    g_shaderFrontFinal.unbind();

    CHECK_GL_ERRORS;
}

//--------------------------------------------------------------------------
void RenderAverageColors()
{
    glDisable(GL_DEPTH_TEST);

    // ---------------------------------------------------------------------
    // 1. Accumulate Colors and Depth Complexity
    // ---------------------------------------------------------------------

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, g_accumulationFboId);
    glDrawBuffers(2, g_drawBuffers);

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    glBlendEquationEXT(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);
    glEnable(GL_BLEND);

    g_shaderAverageInit.bind();
    g_shaderAverageInit.setUniform("Alpha", (float*)&g_opacity, 1);
    DrawModel();
    g_shaderAverageInit.unbind();

    glDisable(GL_BLEND);

    CHECK_GL_ERRORS;

    // ---------------------------------------------------------------------
    // 2. Approximate Blending
    // ---------------------------------------------------------------------

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    glDrawBuffer(GL_BACK);

    g_shaderAverageFinal.bind();
    g_shaderAverageFinal.setUniform("BackgroundColor", g_backgroundColor, 3);
    g_shaderAverageFinal.bindTextureRECT("ColorTex0", g_accumulationTexId[0], 0);
    g_shaderAverageFinal.bindTextureRECT("ColorTex1", g_accumulationTexId[1], 1);
    glCallList(g_quadDisplayList);
    g_shaderAverageFinal.unbind();

    CHECK_GL_ERRORS;
}

//--------------------------------------------------------------------------
void RenderWeightedSum()
{
    glDisable(GL_DEPTH_TEST);

    // ---------------------------------------------------------------------
    // 1. Accumulate (alpha * color) and (alpha)
    // ---------------------------------------------------------------------

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, g_accumulationFboId);
    glDrawBuffer(g_drawBuffers[0]);

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    glBlendEquationEXT(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);
    glEnable(GL_BLEND);

    g_shaderWeightedSumInit.bind();
    g_shaderWeightedSumInit.setUniform("Alpha", (float*)&g_opacity, 1);
    DrawModel();
    g_shaderWeightedSumInit.unbind();

    glDisable(GL_BLEND);

    CHECK_GL_ERRORS;

    // ---------------------------------------------------------------------
    // 2. Weighted Sum
    // ---------------------------------------------------------------------

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    glDrawBuffer(GL_BACK);

    g_shaderWeightedSumFinal.bind();
    g_shaderWeightedSumFinal.setUniform("BackgroundColor", g_backgroundColor, 3);
    g_shaderWeightedSumFinal.bindTextureRECT("ColorTex", g_accumulationTexId[0], 0);
    glCallList(g_quadDisplayList);
    g_shaderWeightedSumFinal.unbind();

    CHECK_GL_ERRORS;
}

//--------------------------------------------------------------------------
void display()
{
    static double s_t0 = currentSeconds();
    g_numGeoPasses = 0;

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(g_pos.x, g_pos.y, g_pos.z, g_pos.x, g_pos.y, 0, 0, 1, 0);
    glRotatef(g_rot.x, 1, 0, 0);
    glRotatef(g_rot.y, 0, 1, 0);
    glTranslatef(g_bbTrans.x, g_bbTrans.y, g_bbTrans.z);
    glScalef(g_bbScale, g_bbScale, g_bbScale);

    switch (g_mode) {
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

    static unsigned s_N = 0;
    s_N++;

    static float s_fps;
    double t1 = currentSeconds();
    double elapsedTime = t1 - s_t0;
    if (elapsedTime > FPS_TIME_WINDOW) {
        s_fps = static_cast<float>(s_N / elapsedTime);
        s_N = 0;
        s_t0 = t1;
    }

    if (g_showOsd) {
        DrawOsd(g_mode, g_opacity, g_numGeoPasses, s_fps);
    }

    glutSwapBuffers();
}

//--------------------------------------------------------------------------
void reshape(int w, int h)
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

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(FOVY, (float)g_imageWidth/(float)g_imageHeight, ZNEAR, ZFAR);
    glMatrixMode(GL_MODELVIEW);

    glViewport(0, 0, g_imageWidth, g_imageHeight);
}

//--------------------------------------------------------------------------
void idle()
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
    float *pixels = new float[3*g_imageWidth*g_imageHeight];
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, g_imageWidth, g_imageHeight, GL_RGB, GL_FLOAT, pixels);

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
    printf("dual_depth_peeling - sample comparing multiple order independent transparency techniques\n");
    printf("  Commands:\n");
    printf("     A/D       - Change uniform opacity\n");
    printf("     1         - Dual peeling mode\n");
    printf("     2         - Front to back peeling mode\n");
    printf("     3         - Weighted average mode\n");
    printf("     4         - Weighted sum mode\n");
    printf("     R         - Reload all shaders\n");
    printf("     B         - Change background color\n");
    printf("     Q         - Toggle occlusion queries\n");
    printf("     +/-       - Change number of geometry passes\n\n");

    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    glutInitWindowSize(g_imageWidth, g_imageHeight);
    glutInit(&argc, argv);
    glutCreateWindow("Dual Depth Peeling");

    glutInitContextVersion(2, 0);
    glutInitContextProfile(GLUT_CORE_PROFILE);

#ifndef __APPLE__
    if (glewInit() != GLEW_OK)
    {
        printf("glewInit failed. Exiting...\n");
        exit(1);
    }

    if (!glewIsSupported( "GL_VERSION_2_0 "
                          "GL_ARB_texture_rectangle "
                          "GL_ARB_texture_float "
                          "GL_ARB_color_buffer_float "
                          "GL_ARB_depth_buffer_float "))
    {
        printf("Unable to load the necessary extensions\n");
        printf("This sample requires:\n");
        printf("OpenGL version 2.0\n");
        printf("GL_ARB_texture_rectangle\n");
        printf("GL_ARB_texture_float\n");
        printf("GL_ARB_color_buffer_float\n");
        printf("GL_ARB_depth_buffer_float\n");
        printf("Exiting...\n");
        exit(1);
    }
#endif

    printf("GL version %s\n", glGetString(GL_VERSION));
    printf("GL vendor %s\n", glGetString(GL_VENDOR));
    printf("GL render %s\n", glGetString(GL_RENDERER));
    printf("GLSL version %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

    InitGL();
    InitMenus();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);
    glutMouseFunc(mouseFunc);
    glutMotionFunc(motionFunc);
    glutKeyboardFunc(keyboardFunc);

    glutMainLoop();
    return 0;
}
