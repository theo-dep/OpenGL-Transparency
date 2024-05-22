//
//
// Author: Louis Bavoil
// Email: sdkfeedback@nvidia.com
//
// Copyright (c) NVIDIA Corporation. All rights reserved.
////////////////////////////////////////////////////////////////////////////////

#ifndef OSD_H
#define OSD_H


enum {
	NORMAL_BLENDING_MODE,
	DUAL_PEELING_MODE,
	F2B_PEELING_MODE,
	WEIGHTED_AVERAGE_MODE,
	WEIGHTED_SUM_MODE
};

#if 0
#define CHECK_GL_ERRORS  \
{ \
    GLenum err = glGetError(); \
    if (err) \
        printf( "Error %s at line %d\n", gluErrorString(err), __LINE__); \
}
#else
#define CHECK_GL_ERRORS {}
#endif

void InitFullScreenQuad();
void DeleteFullScreenQuad();
void DrawFullScreenQuad();

void LoadShaderText();
void DestroyShaderText();

void InitText();
void DeleteText();

void DrawOsd(char mode, float opacity, int numPasses, float fps);

#endif
