#pragma once

#include <vector>

#include "shader/shader.h"

class TextureArray {
	public:
		GLuint ID;
		int width, height, channels;
		TextureArray(const std::vector<std::string>& texPaths, int w, int h, int ch);
		~TextureArray();

		void texUnit(Shader& shader, const char* uniform, GLuint unit);
		void bind();
		void unbind();
		void remove();
};