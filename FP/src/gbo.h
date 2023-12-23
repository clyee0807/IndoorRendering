#pragma once

#include <string>

enum GBOTextureType {
    GBO_TEXTURE_TYPE_VERTEX   = 0,
    GBO_TEXTURE_TYPE_NORMAL   = 1,
    GBO_TEXTURE_TYPE_DIFFUSE  = 2,
    GBO_TEXTURE_TYPE_AMBIENT  = 3,
    GBO_TEXTURE_TYPE_SPECULAR = 4,
    GBO_TEXTURE_TYPE_DEPTH    = 5,
    GBO_NUM_TEXTURES          = 6,
};

class GBO {
    private:
        GLuint gbo, texture[GBO_NUM_TEXTURES];
        std::string texType[GBO_NUM_TEXTURES] = {
            "COORDS", "COORDS", "COLORS", "COLORS", "COLORS", "DEPTH"
        };
        int width, height;

    public:
        GBO(int w, int h) : gbo(0), width(w), height(h) {}

        ~GBO() { release(); }

        void init() { init(this->width, this->height); }

        void init(int w, int h) {
            // Ensure it updates all parameters everytime
            this->width = w;
            this->height = h;

            // Framebuffer
            glGenFramebuffers(1, &gbo);
            glBindFramebuffer(GL_FRAMEBUFFER, gbo);

            // Create textures
            glGenTextures(GBO_NUM_TEXTURES, texture);

            for (int i = 0; i < GBO_NUM_TEXTURES; i++) {
                glBindTexture(GL_TEXTURE_2D, texture[i]);

                if (texType[i] != "DEPTH") {
                    if (texType[i] == "COORDS")
                        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, w, h);
                    else if (texType[i] == "COLORS")
                        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, w, h);

                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, texture[i], 0);
                } else {
                    glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, w, h);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture[i], 0);
                }
            }

            // Draw buffers
            const int NUM_ATTACHMENTS = GBO_NUM_TEXTURES - 1;
            GLenum attachments[NUM_ATTACHMENTS] = {};
            for (int i = 0; i < NUM_ATTACHMENTS; i++) {
                attachments[i] = GL_COLOR_ATTACHMENT0 + i;
            }
            glDrawBuffers(NUM_ATTACHMENTS, attachments);

            auto gboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (gboStatus != GL_FRAMEBUFFER_COMPLETE) {
                std::cerr << "GB::ERROR: " << gboStatus << std::endl;
            }

            // Unbind the framebuffer
            glBindTexture(GL_TEXTURE_2D, 0);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        void resize(int newWidth, int newHeight) {
            if (width == newWidth && height == newHeight)
                return;

            // Delete old framebuffer and texture
            release();

            // Reinitialize with the new size
            init(newWidth, newHeight);
        }

        void bindWrite() const {
            glBindFramebuffer(GL_FRAMEBUFFER, gbo);
            glClearColor(0.19f, 0.19f, 0.19f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }

        void bindRead() const {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, gbo);
        }

        GLuint getTexture(GBOTextureType texType) const {
            return texture[static_cast<int>(texType)];
        }

        void setReadBuffer(GBOTextureType texType) const {
            glReadBuffer(GL_COLOR_ATTACHMENT0 + static_cast<int>(texType));
        }

        static void unbind() {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

    private:
        void release() {
            if (texture) {
                for (int i = 0; i < GBO_NUM_TEXTURES; i++) {
                    glDeleteTextures(1, &texture[i]);
                    texture[i] = 0;
                }
            }

            if (gbo) {
                glDeleteFramebuffers(1, &gbo);
                gbo = 0;
            }
        }
};