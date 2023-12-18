#include "shader.h"

namespace ShaderUtils {
	std::string getFileContents(const char* filename) {
		ifstream in(filename, ios::binary);
		if (in) {
			string contents;
			in.seekg(0, ios::end);
			contents.resize(in.tellg());
			in.seekg(0, ios::beg);
			in.read(&contents[0], contents.size());
			in.close();
			return(contents);
		}
		throw(errno);
	}

	void compileErrors(unsigned int shader, const char* type) {
		// Stores status of compilation
		GLint hasCompiled;
		// Character array to store error message in
		char infoLog[1024];
		if (type != "PROGRAM") {
			glGetShaderiv(shader, GL_COMPILE_STATUS, &hasCompiled);
			if (hasCompiled == GL_FALSE) {
				glGetShaderInfoLog(shader, 1024, NULL, infoLog);
				cout << "SHADER_COMPILATION_ERROR for:" << type << "\n" << infoLog << endl;
			}
		}
		else {
			glGetProgramiv(shader, GL_LINK_STATUS, &hasCompiled);
			if (hasCompiled == GL_FALSE) {
				glGetProgramInfoLog(shader, 1024, NULL, infoLog);
				cout << "SHADER_LINKING_ERROR for:" << type << "\n" << infoLog << endl;
			}
		}
	}
}

/* ---------------------------- NORM ---------------------------- */
Shader::Shader(const char* vertexFile, const char* fragmentFile) {
	// Read vertexFile and fragmentFile and store the strings
	string vertexCode = ShaderUtils::getFileContents(vertexFile);
	string fragmentCode = ShaderUtils::getFileContents(fragmentFile);

	// Convert the shader source strings into character arrays
	const char* vertexSource = vertexCode.c_str();
	const char* fragmentSource = fragmentCode.c_str();

	// Create Vertex Shader Object and get its reference
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	// Attach Vertex Shader source to the Vertex Shader Object
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	// Compile the Vertex Shader into machine code
	glCompileShader(vertexShader);
	// Checks if Shader compiled succesfully
	ShaderUtils::compileErrors(vertexShader, "VERTEX");

	// Create Fragment Shader Object and get its reference
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	// Attach Fragment Shader source to the Fragment Shader Object
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	// Compile the Vertex Shader into machine code
	glCompileShader(fragmentShader);
	// Checks if Shader compiled succesfully
	ShaderUtils::compileErrors(fragmentShader, "FRAGMENT");

	// Create Shader Program Object and get its reference
	ID = glCreateProgram();
	// Attach the Vertex and Fragment Shaders to the Shader Program
	glAttachShader(ID, vertexShader);
	glAttachShader(ID, fragmentShader);
	// Wrap-up/Link all the shaders together into the Shader Program
	glLinkProgram(ID);
	// Checks if Shaders linked succesfully
	ShaderUtils::compileErrors(ID, "PROGRAM");

	// Delete the now useless Vertex and Fragment Shader objects
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
}

void Shader::activate() {
	glUseProgram(ID);
}

void Shader::remove() {
	glDeleteProgram(ID);
}



/* ---------------------------- COMP ---------------------------- */
ComputeShader::ComputeShader(const char* computeFile) {
	// Read computeFile and store the string
	string computeCode = ShaderUtils::getFileContents(computeFile);

	// Convert the shader source string into a character array
	const char* computeSource = computeCode.c_str();

	// Create Compute Shader Object and get its reference
	GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
	// Attach Compute Shader source to the Compute Shader Object
	glShaderSource(computeShader, 1, &computeSource, NULL);
	// Compile the Compute Shader into machine code
	glCompileShader(computeShader);
	// Check if Shader compiled successfully
	ShaderUtils::compileErrors(computeShader, "COMPUTE");

	// Create Shader Program Object and get its reference
	ID = glCreateProgram();
	// Attach the Compute Shader to the Shader Program
	glAttachShader(ID, computeShader);
	// Link all the shaders together into the Shader Program
	glLinkProgram(ID);
	// Check if Shaders linked successfully
	ShaderUtils::compileErrors(ID, "PROGRAM");

	// Delete the now useless Compute Shader object
	glDeleteShader(computeShader);
}
void ComputeShader::activate() {
	glUseProgram(ID);
}

void ComputeShader::remove() {
	glDeleteProgram(ID);
}