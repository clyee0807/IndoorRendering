#include "shader.h"

/* ---------------------------- SP ---------------------------- */
std::string ShaderProgram::getFileContents(const char* filename) {
	std::ifstream in(filename, std::ios::binary);
	if (in) {
		std::string contents;
		in.seekg(0, std::ios::end);
		contents.resize(in.tellg());
		in.seekg(0, std::ios::beg);
		in.read(&contents[0], contents.size());
		in.close();
		return(contents);
	}
	throw(errno);
}

void ShaderProgram::compileShader(const char* source, GLenum shaderType, std::string typeStr) {
	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);
	compileErrors(shader, typeStr.c_str());

	glAttachShader(ID, shader);
	glDeleteShader(shader);
}

void ShaderProgram::compileErrors(unsigned int shader, const char* type) {
	GLint hasCompiled;
	char infoLog[1024];

	if (type != "PROGRAM") {
		glGetShaderiv(shader, GL_COMPILE_STATUS, &hasCompiled);
		if (hasCompiled == GL_FALSE) {
			glGetShaderInfoLog(shader, 1024, NULL, infoLog);
			std::cerr << "SHADER::" << type << "::COMPILATION_ERROR: " << infoLog << std::endl;
		}
	} else {
		glGetProgramiv(shader, GL_LINK_STATUS, &hasCompiled);
		if (hasCompiled == GL_FALSE) {
			glGetProgramInfoLog(shader, 1024, NULL, infoLog);
			std::cerr << "SHADER::" << type << "::LINK_ERROR: " << infoLog << std::endl;
		}
	}
}

void ShaderProgram::linkProgram() {
	glLinkProgram(ID);
	compileErrors(ID, "PROGRAM");
}

void ShaderProgram::activate() const {
	glUseProgram(ID);
}

GLint ShaderProgram::getUniformLocation(const std::string& name) const {
	auto it = uniformLocations.find(name);
	if (it == uniformLocations.end()) {
		// Insert new element in a const context
		GLint location = glGetUniformLocation(ID, name.c_str());
		uniformLocations.insert(std::make_pair(name, location));
		return location;
	}
	return it->second;
}

GLuint ShaderProgram::getID() const {
	return ID;
}

void ShaderProgram::remove() {
	if (ID) {
		glDeleteProgram(ID);
		ID = 0;
	}
}

void ShaderProgram::setMat4(const std::string& name, const glm::mat4& mat) const {
	glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(mat));
}

void ShaderProgram::setVec4(const std::string& name, const glm::vec4& vec) const {
	glUniform4fv(getUniformLocation(name), 1, glm::value_ptr(vec));
}

void ShaderProgram::setVec3(const std::string& name, const glm::vec3& vec) const {
	glUniform3fv(getUniformLocation(name), 1, glm::value_ptr(vec));
}

void ShaderProgram::setFloat(const std::string& name, float value) const {
	glUniform1f(getUniformLocation(name), value);
}

void ShaderProgram::setInt(const std::string& name, int value) const {
	glUniform1i(getUniformLocation(name), value);
}

void ShaderProgram::setTexture(const std::string& name, GLuint textureID, GLuint textureUnit) const {
	glActiveTexture(GL_TEXTURE0 + textureUnit);
	glBindTexture(GL_TEXTURE_2D, textureID);
	setInt(name, textureUnit);
}



/* --------------------------- EXTEND --------------------------- */
Shader::Shader(const char* vertexFile, const char* fragmentFile) {
	ID = glCreateProgram();

	std::string vertexCodeStr = ShaderProgram::getFileContents(vertexFile);
	std::string fragmentCodeStr = ShaderProgram::getFileContents(fragmentFile);
	const char* vertexCode = vertexCodeStr.c_str();
	const char* fragmentCode = fragmentCodeStr.c_str();

	compileShader(vertexCode, GL_VERTEX_SHADER, "VERTEX");
	compileShader(fragmentCode, GL_FRAGMENT_SHADER, "FRAGMENT");

	linkProgram();
}

ComputeShader::ComputeShader(const char* computeFile) {
	ID = glCreateProgram();

	std::string computeCodeStr = ShaderProgram::getFileContents(computeFile);
	const char* computeCode = computeCodeStr.c_str();
	compileShader(computeCode, GL_COMPUTE_SHADER, "COMPUTE");

	linkProgram();
}