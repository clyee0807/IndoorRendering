#include "texture.h"

#include <glad/glad.h>
#include <stb/stb_image.h>

Texture::Texture(const char* imagePath) : ID(0) {
	int width, height, numColCh;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* imageData = stbi_load(imagePath, &width, &height, &numColCh, 0);

	glGenTextures(1, &ID);

	if (imageData) {
		GLenum format{};
		if (numColCh == 1) format = GL_RED;
		else if (numColCh == 3) format = GL_RGB;
		else if (numColCh == 4) format = GL_RGBA;

		bind();
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, imageData);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (numColCh == 1) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
		}

		std::cout << "  - IMAGE OK (CH: " << numColCh << "): " << imagePath << std::endl;
	} else {
		std::cerr << "  - IMAGE::FAILED:: " << imagePath << std::endl;
	}

	stbi_image_free(imageData);
	unbind();
}

void Texture::texUnit(Shader& shader, const char* uniform, GLuint unit) {
	shader.activate();
	shader.setInt(uniform, unit);
}

void Texture::bind() {
	glBindTexture(GL_TEXTURE_2D, ID);
}

void Texture::unbind() {
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::remove() {
	glDeleteTextures(1, &ID);
}