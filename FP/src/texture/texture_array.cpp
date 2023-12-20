#include "texture_array.h"

#include <glad/glad.h>
#include <stb/stb_image.h>

TextureArray::TextureArray(const std::vector<std::string>& texPaths, int w, int h, int ch)
	: width(w), height(h), channels(ch) {

	glGenTextures(1, &ID);
	bind();

	const size_t layerSize = width * height * channels;
	unsigned char* texArrayData = new unsigned char[layerSize * texPaths.size()];

	stbi_set_flip_vertically_on_load(true);
	for (size_t i = 0; i < texPaths.size(); ++i) {
		int texWidth, texHeight, texChannels;
		unsigned char* texData = stbi_load(texPaths[i].c_str(), &texWidth, &texHeight, &texChannels, channels);

		if (texData && texWidth == width && texHeight == height && texChannels == channels) {
			memcpy(texArrayData + (i * layerSize), texData, layerSize);
			stbi_image_free(texData);
			std::cout << "   Texture loaded OK: " << texPaths[i] << std::endl;
		} else {
			delete[] texArrayData;
			texArrayData = nullptr;
			std::cerr << "   Texture failed to load: " << texPaths[i] << std::endl;
			return;
		}
	}

	GLuint level = log2(std::max(width, height)) + 1;
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, level, (channels == 4 ? GL_RGBA8 : GL_RGB8), width, height, texPaths.size());
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, width, height, texPaths.size(), (channels == 4 ? GL_RGBA : GL_RGB), GL_UNSIGNED_BYTE, texArrayData);
	delete[] texArrayData;

	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

	unbind();
}

TextureArray::~TextureArray() {
	glDeleteTextures(1, &ID);
}

void TextureArray::texUnit(Shader& shader, const char* uniform, GLuint unit) {
	shader.activate();
	glActiveTexture(GL_TEXTURE0 + unit);
	bind();
	shader.setInt(uniform, unit);
}

void TextureArray::bind() {
	glBindTexture(GL_TEXTURE_2D_ARRAY, ID);
}

void TextureArray::unbind() {
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

void TextureArray::remove() {
	glDeleteTextures(1, &ID);
}