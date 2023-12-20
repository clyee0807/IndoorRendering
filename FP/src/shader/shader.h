#ifndef SHADER_CLASS_H
#define SHADER_CLASS_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <string>
#include <cerrno>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>



class ShaderProgram {
	protected:
		GLuint ID;
		mutable std::unordered_map<std::string, GLint> uniformLocations;

		std::string getFileContents(const char* filename);
		void compileShader(const char* source, GLenum shaderType, std::string typeStr);
		void compileErrors(unsigned int shader, const char* type);
		void linkProgram();

	public:
		ShaderProgram() : ID(0) {}
		~ShaderProgram() { remove(); }

		void activate() const;
		GLint getUniformLocation(const std::string& name) const;
		GLuint getID() const;
		void remove();

		void setMat4(const std::string& name, const glm::mat4& mat) const;
		void setVec4(const std::string& name, const glm::vec4& vec) const;
		void setVec3(const std::string& name, const glm::vec3& vec) const;
		void setFloat(const std::string& name, float value) const;
		void setInt(const std::string& name, int value) const;

		void setTexture(const std::string& uniformName, GLuint textureID, GLuint textureUnit) const;
};

class Shader : public ShaderProgram {
	public:
		Shader(const char* vertexFile, const char* fragmentFile);
};

class ComputeShader : public ShaderProgram {
	public:
		ComputeShader(const char* computeFile);
};



#endif