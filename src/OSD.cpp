//
//
// Author: Louis Bavoil
// Email: sdkfeedback@nvidia.com
//
// Copyright (c) NVIDIA Corporation. All rights reserved.
////////////////////////////////////////////////////////////////////////////////

#pragma warning( disable : 4996 )

#include "OSD.h"

#include <GL/glew.h>
#include <GL/glut.h>

#include <iostream>
#include <sstream>

#define NUM_LINES 4

#if 0
//--------------------------------------------------------------------------
void DrawText(int x, int y, const char *text)
{
	glRasterPos2i(x, y);
	while (*text)
	{
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *text);
		text++;
	}
}

//--------------------------------------------------------------------------
void DrawOsd(char mode, float opacity, int numPasses, float fps)
{
	int imageWidth = glutGet((GLenum)GLUT_WINDOW_WIDTH);
	int imageHeight = glutGet((GLenum)GLUT_WINDOW_HEIGHT);

	glDisable(GL_DEPTH_TEST);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0, imageWidth, 0, imageHeight);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	int x1 = 0;
	int x2 = 128;
	int w = x2-x1;
	int y1 = 0;
	int lineHeight = 15;
	int margin = 5;
	int y2 = y1 + margin + lineHeight * NUM_LINES;

	glBegin(GL_QUADS);
	{
		glColor3f(.2f,.2f,.2f);
		glVertex2f(static_cast<float>(x1), static_cast<float>(y1));
		glVertex2f(static_cast<float>(x1+w), static_cast<float>(y1));
		glVertex2f(static_cast<float>(x1+w), static_cast<float>(y2));
		glVertex2f(static_cast<float>(x1), static_cast<float>(y2));
	}
	glEnd();

	glColor3f(1,1,1);

	glPushAttrib(GL_ENABLE_BIT | GL_VIEWPORT_BIT);
	char buf[256];

	x1 += margin;
	y1 += margin;

	sprintf(buf, "Frame Rate: %.0f fps", fps);
	DrawText(x1, y1, buf);
	y1 += lineHeight;

	sprintf(buf, "Geometry Passes: %d", numPasses);
	DrawText(x1, y1, buf);
	y1 += lineHeight;

	sprintf(buf, "Opacity: %.0f%%", opacity*100);
	DrawText(x1, y1, buf);
	y1 += lineHeight;

	switch (mode) {
		case DUAL_PEELING_MODE:
			sprintf(buf, "Dual Peeling");
			break;
		case F2B_PEELING_MODE:
			sprintf(buf, "Front Peeling");
			break;
		case WEIGHTED_AVERAGE_MODE:
			sprintf(buf, "Weighted Average");
			break;
		case WEIGHTED_SUM_MODE:
			sprintf(buf, "Weighted Sum");
			break;
	}

	DrawText(x1, y1, buf);
	y1 += lineHeight;

	glPopAttrib();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}
#endif
