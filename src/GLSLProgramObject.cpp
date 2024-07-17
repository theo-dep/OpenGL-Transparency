//
// GLSLProgramObject.cpp - Wrapper for GLSL program objects
//
// Author: Louis Bavoil
// Email: sdkfeedback@nvidia.com
//
// Copyright (c) NVIDIA Corporation. All rights reserved.
////////////////////////////////////////////////////////////////////////////////

#include "GLSLProgramObject.h"

#include <glm/gtc/type_ptr.hpp>

#include <cmrc/cmrc.hpp>
CMRC_DECLARE(shaders);

#include <iostream>

GLSLProgramObject::GLSLProgramObject() :
	_progId(0)
{
}

GLSLProgramObject::~GLSLProgramObject()
{
	destroy();
}

void GLSLProgramObject::destroy()
{
	for (unsigned i = 0; i < _vertexShaders.size(); i++) {
		glDeleteShader(_vertexShaders[i]);
	}
	for (unsigned i = 0; i < _fragmentShaders.size(); i++) {
		glDeleteShader(_fragmentShaders[i]);
	}
	if (_progId != 0) {
		glDeleteProgram(_progId);
	}
}

void GLSLProgramObject::attachVertexShader(const std::string& filename)
{
	std::cout << filename << std::endl;

    auto shaderFilesystem = cmrc::shaders::get_filesystem();
    if (!shaderFilesystem.exists("shaders/" + filename)) {
        std::cerr << "Error: Failed to find vertex shader" << std::endl;
		exit(1);
    }

    GLuint shaderId = glCreateShader(GL_VERTEX_SHADER);
    if (shaderId == 0) {
	    std::cerr << "Error: Vertex shader failed to create" << std::endl;
	    exit(1);
    }

    auto shaderFile = shaderFilesystem.open("shaders/" + filename);
	auto shader = std::string(shaderFile.cbegin(), shaderFile.cend());
	const char *shaderStr = shader.c_str();

	glShaderSource(shaderId, 1, &shaderStr, NULL);
    glCompileShader(shaderId);

    _vertexShaders.push_back(shaderId);
}

void GLSLProgramObject::attachFragmentShader(const std::string& filename)
{
	std::cout << filename << std::endl;

    auto shaderFilesystem = cmrc::shaders::get_filesystem();
    if (!shaderFilesystem.exists("shaders/" + filename)) {
        std::cerr << "Error: Failed to find fragment shader" << std::endl;
		exit(1);
    }

    GLuint shaderId = glCreateShader(GL_FRAGMENT_SHADER);
    if (shaderId == 0) {
	    std::cerr << "Error: Fragment shader failed to create" << std::endl;
	    exit(1);
    }

    auto shaderFile = shaderFilesystem.open("shaders/" + filename);
	auto shader = std::string(shaderFile.cbegin(), shaderFile.cend());
	const char *shaderStr = shader.c_str();

	glShaderSource(shaderId, 1, &shaderStr, NULL);
    glCompileShader(shaderId);

    _fragmentShaders.push_back(shaderId);
}

void GLSLProgramObject::link()
{
	_progId = glCreateProgram();

    for (unsigned i = 0; i < _vertexShaders.size(); i++) {
        glAttachShader(_progId, _vertexShaders[i]);
    }

    for (unsigned i = 0; i < _fragmentShaders.size(); i++) {
        glAttachShader(_progId, _fragmentShaders[i]);
    }

    glLinkProgram(_progId);

    GLint success = 0;
    glGetProgramiv(_progId, GL_LINK_STATUS, &success);

    if (!success) {
        char temp[1024];
        glGetProgramInfoLog(_progId, 1024, NULL, temp);
        std::cerr << "Failed to link program:" << std::endl;
        std::cerr << temp << std::endl;
		exit(1);
    }
}

void GLSLProgramObject::bind()
{
	glUseProgram(_progId);
}

void GLSLProgramObject::unbind()
{
	glUseProgram(0);
}

void GLSLProgramObject::setUniform(const std::string& name, GLuint val)
{
	GLint id = glGetUniformLocation(_progId, name.c_str());
	glUniform1uiv(id, 1, &val);
}

void GLSLProgramObject::setUniform(const std::string& name, GLfloat val)
{
	GLint id = glGetUniformLocation(_progId, name.c_str());
	glUniform1fv(id, 1, &val);
}

void GLSLProgramObject::setUniform(const std::string& name, const glm::vec3& val)
{
	GLint id = glGetUniformLocation(_progId, name.c_str());
	glUniform3fv(id, 1, glm::value_ptr(val));
}

void GLSLProgramObject::setUniform(const std::string& name, const glm::mat3& val)
{
	GLint id = glGetUniformLocation(_progId, name.c_str());
	glUniformMatrix3fv(id, 1, GL_FALSE, glm::value_ptr(val));
}

void GLSLProgramObject::setUniform(const std::string& name, const glm::mat4& val)
{
	GLint id = glGetUniformLocation(_progId, name.c_str());
	glUniformMatrix4fv(id, 1, GL_FALSE, glm::value_ptr(val));
}

void GLSLProgramObject::setTextureUnit(const std::string& texname, int texunit)
{
	GLint linked;
	glGetProgramiv(_progId, GL_LINK_STATUS, &linked);
	if (linked != GL_TRUE) {
		std::cerr << "Error: setTextureUnit needs program to be linked." << std::endl;
		exit(1);
	}
	GLint id = glGetUniformLocation(_progId, texname.c_str());
	if (id == -1) {
		std::cerr << "Warning: Invalid texture " << texname << std::endl;
		return;
	}
	glUniform1i(id, texunit);
}

void GLSLProgramObject::bindTexture(GLenum target, const std::string& texname, GLuint texid, int texunit)
{
	glActiveTexture(GL_TEXTURE0 + texunit);
	glBindTexture(target, texid);
	setTextureUnit(texname, texunit);
	glActiveTexture(GL_TEXTURE0);
}
