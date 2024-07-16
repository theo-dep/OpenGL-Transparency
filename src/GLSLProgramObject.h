//
// GLSLProgramObject.h - Wrapper for GLSL program objects
//
// Author: Louis Bavoil
// Email: sdkfeedback@nvidia.com
//
// Copyright (c) NVIDIA Corporation. All rights reserved.
////////////////////////////////////////////////////////////////////////////////

#ifndef GLSL_PROGRAM_OBJECT_H
#define GLSL_PROGRAM_OBJECT_H

#include <GL/glew.h>

#include <glm/vec3.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>

#include <string>
#include <vector>

class GLSLProgramObject
{
public:
	GLSLProgramObject();
	virtual ~GLSLProgramObject();

	void destroy();

	void bind();

	void unbind();

	void setUniform(const std::string& name, GLuint val);
	void setUniform(const std::string& name, GLfloat val);
	void setUniform(const std::string& name, const glm::vec3& val);
	void setUniform(const std::string& name, const glm::mat3& val);
	void setUniform(const std::string& name, const glm::mat4& val);

	void setTextureUnit(const std::string& texname, int texunit);

	void bindTexture(GLenum target, const std::string& texname, GLuint texid, int texunit);

	void bindTexture2D(const std::string& texname, GLuint texid, int texunit) {
		bindTexture(GL_TEXTURE_2D, texname, texid, texunit);
	}

	void bindTexture3D(const std::string& texname, GLuint texid, int texunit) {
		bindTexture(GL_TEXTURE_3D, texname, texid, texunit);
	}

	void bindTextureRECT(const std::string& texname, GLuint texid, int texunit) {
		bindTexture(GL_TEXTURE_RECTANGLE_ARB, texname, texid, texunit);
	}

	void attachVertexShader(const std::string& filename);

	void attachFragmentShader(const std::string& filename);

	void link();

	inline GLuint getProgId() { return _progId; }

protected:
	std::vector<GLuint>		_vertexShaders;
	std::vector<GLuint>		_fragmentShaders;
	GLuint					_progId;
};

#endif
