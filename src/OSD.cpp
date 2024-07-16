//
//
// Author: Louis Bavoil
// Email: sdkfeedback@nvidia.com
//
// Copyright (c) NVIDIA Corporation. All rights reserved.
////////////////////////////////////////////////////////////////////////////////

#pragma warning( disable : 4996 )

#include "OSD.h"
#include "GLSLProgramObject.h"

#include <GL/glew.h>
#include <GL/glut.h>

#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <iostream>
#include <string>
#include <array>
#include <map>
#include <filesystem>

GLuint g_quadVaoId;
GLuint g_quadVboId;

GLuint g_textQuadVaoId;
GLuint g_textQuadVboId;

/// Holds all state information relevant to a character as loaded using FreeType
struct Character {
    unsigned int TextureID; // ID handle of the glyph texture
    glm::ivec2   Size;      // Size of glyph
    glm::ivec2   Bearing;   // Offset from baseline to left/top of glyph
    unsigned int Advance;   // Horizontal offset to advance to next glyph
};

std::map<GLchar, Character> g_characters;
GLSLProgramObject g_shaderText;

//--------------------------------------------------------------------------
void InitFullScreenQuad()
{
    glGenVertexArrays(1, &g_quadVaoId);
    glGenBuffers(1, &g_quadVboId);

    glBindVertexArray(g_quadVaoId);
    glBindBuffer(GL_ARRAY_BUFFER, g_quadVboId);

    constexpr std::array quadBufferData{
        -1.f, -1.f, 1.f, -1.f, 1.f, 1.f, -1.f, 1.f
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadBufferData), quadBufferData.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (GLubyte*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (GLubyte*)sizeof(quadBufferData));

    CHECK_GL_ERRORS;
}

//--------------------------------------------------------------------------
void DeleteFullScreenQuad()
{
	glDeleteVertexArrays(1, &g_quadVaoId);
	glDeleteBuffers(1, &g_quadVboId);

    CHECK_GL_ERRORS;
}

//--------------------------------------------------------------------------
void DrawFullScreenQuad()
{
    glBindVertexArray(g_quadVaoId);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    CHECK_GL_ERRORS;
}

//--------------------------------------------------------------------------
void LoadShaderText()
{
	g_shaderText.attachVertexShader("text_vertex.glsl");
    g_shaderText.attachFragmentShader("text_fragment.glsl");
    g_shaderText.link();
}

//--------------------------------------------------------------------------
void DestroyShaderText()
{
	g_shaderText.destroy();
}

//--------------------------------------------------------------------------
void InitText()
{
	// FreeType
    // --------
    FT_Library ft;
    // All functions return a value different than 0 whenever an error occurred
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        exit(1);
    }

	// find path to font
    std::string fontName = std::filesystem::canonical("fonts/Antonio-Regular.ttf").string();
    if (fontName.empty())
    {
        std::cout << "ERROR::FREETYPE: Failed to load font_name" << std::endl;
        exit(1);
    }

	// load font as face
    FT_Face face;
    if (FT_New_Face(ft, fontName.c_str(), 0, &face)) {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
        exit(1);
    }
    else {
        // set size to load glyphs as
        FT_Set_Pixel_Sizes(face, 0, 48);

        // disable byte-alignment restriction
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        // load first 128 characters of ASCII set
        for (unsigned char c = 0; c < 128; c++)
        {
            // Load character glyph
            if (FT_Load_Char(face, c, FT_LOAD_RENDER))
            {
                std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
                continue;
            }
            // generate texture
            unsigned int texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
            );
            // set texture options
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            // now store character for later use
            Character character = {
                texture,
                glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                static_cast<unsigned int>(face->glyph->advance.x)
            };
            g_characters.insert(std::pair<char, Character>(c, character));
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    // destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    // configure VAO/VBO for texture quads
    // -----------------------------------
    glGenVertexArrays(1, &g_textQuadVaoId);
    glGenBuffers(1, &g_textQuadVboId);
    glBindVertexArray(g_textQuadVaoId);
    glBindBuffer(GL_ARRAY_BUFFER, g_textQuadVboId);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    CHECK_GL_ERRORS;
}

//--------------------------------------------------------------------------
void DeleteText()
{
    for (const auto& [c, character] : g_characters)
    {
        glDeleteTextures(1, &character.TextureID);
    }

	glDeleteVertexArrays(1, &g_textQuadVaoId);
	glDeleteBuffers(1, &g_textQuadVboId);

    CHECK_GL_ERRORS;
}

//--------------------------------------------------------------------------
void DrawText(const std::string& text, float x, float y, float scale)
{
    // iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++)
    {
        const Character& ch = g_characters[*c];

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;
        // update VBO for each character
        float vertices[6][4] = {
            { xpos,     ypos + h, 0.0f, 0.0f },
            { xpos,     ypos,     0.0f, 1.0f },
            { xpos + w, ypos,     1.0f, 1.0f },

            { xpos,     ypos + h, 0.0f, 0.0f },
            { xpos + w, ypos,     1.0f, 1.0f },
            { xpos + w, ypos + h, 1.0f, 0.0f }
        };

        // render glyph texture over quad
        g_shaderText.bindTexture2D("Text0", ch.TextureID, 0);
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);

        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, g_textQuadVboId);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // be sure to use glBufferSubData and not glBufferData

        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
    }

    CHECK_GL_ERRORS;
}

//--------------------------------------------------------------------------
void DrawOsd(char mode, float opacity, int numPasses, float fps)
{
	glm::vec3 textColor(.4f, .85f, .0f);
	float scale = 0.4f;

	// activate corresponding render state
    g_shaderText.bind();
    g_shaderText.setUniform("TextColor", textColor);

	int imageWidth = glutGet((GLenum)GLUT_WINDOW_WIDTH);
	int imageHeight = glutGet((GLenum)GLUT_WINDOW_HEIGHT);
	glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(imageWidth), 0.0f, static_cast<float>(imageHeight));
    g_shaderText.setUniform("Projection", projection);

	glBindVertexArray(g_textQuadVaoId);

	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	int x1 = 0;
	int y1 = 0;
	int lineHeight = 20;
	int margin = 5;

	std::string buf;

	x1 += margin;
	y1 += margin;

	buf = "Frame Rate: " + std::to_string(static_cast<int>(std::round(fps))) + " fps";
	DrawText(buf, x1, y1, scale);
	y1 += lineHeight;

	buf = "Geometry Passes: " + std::to_string(numPasses);
	DrawText(buf, x1, y1, scale);
	y1 += lineHeight;

	buf = "Opacity: " + std::to_string(static_cast<int>(std::round(opacity * 100))) + "%";
	DrawText(buf, x1, y1, scale);
	y1 += lineHeight;

	switch (mode) {
	    case NORMAL_BLENDING_MODE:
	        buf = "Normal Blending";
			break;
		case DUAL_PEELING_MODE:
			buf = "Dual Peeling";
			break;
		case F2B_PEELING_MODE:
		    buf = "Front Peeling";
			break;
		case WEIGHTED_AVERAGE_MODE:
			buf = "Weighted Average";
			break;
		case WEIGHTED_SUM_MODE:
			buf = "Weighted Sum";
			break;
		case BSP_MODE:
	    	buf = "Binary Space Partitioning";
            break;
        default:
            buf = "Unknown mode";
	}

	DrawText(buf, x1, y1, scale);
	y1 += lineHeight;

	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);

    CHECK_GL_ERRORS;
}
