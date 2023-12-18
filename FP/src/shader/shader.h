#ifndef SHADER_CLASS_H
#define SHADER_CLASS_H

#include <glad/glad.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cerrno>

using namespace std;

namespace ShaderUtils {
	string getFileContents(const char* filename);
	void compileErrors(unsigned int shader, const char* type);
}

class Shader {
	public:
		GLuint ID;
		Shader(const char* vertexFile, const char* fragmentFile);

		void activate();
		void remove();
};

class ComputeShader {
public:
	GLuint ID;
	ComputeShader(const char* computeFile);

	void activate();
	void remove();
};



#endif