//
//
// Author: Louis Bavoil
// Email: sdkfeedback@nvidia.com
//
// Copyright (c) NVIDIA Corporation. All rights reserved.
////////////////////////////////////////////////////////////////////////////////

#ifndef OSD_H
#define OSD_H

#include <iostream>
#include <sstream>
#include <string>
#include <time.h>
#include <GL/glew.h>
#include <GL/glut.h>

enum {
	DUAL_PEELING_MODE,
	F2B_PEELING_MODE,
	WEIGHTED_AVERAGE_MODE,
	WEIGHTED_SUM_MODE
};

void DrawText(int x, int y, const char *text);
void DrawOsd(char mode, float opacity, int numPasses, float fps);

#endif
