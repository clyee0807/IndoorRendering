#pragma once

#include "shader/shader.h"

class Texture {
	public:
		GLuint ID;
		std::string imageName;
		Texture(const char* imagePath);

		void texUnit(Shader& shader, const char* uniform, GLuint unit);
		void bind();
		void unbind();
		void remove();
};