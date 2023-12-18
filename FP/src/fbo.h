#pragma once

class FBO {
    private:
        GLuint fbo, rbo, texture;
        int width, height;

    public:
        FBO() : fbo(0), rbo(0), texture(0), width(0), height(0) {}

        ~FBO() {
            release();
        }

        void init(int w, int h) {
            width = w;
            height = h;

            // Generate and bind the framebuffer
            glGenFramebuffers(1, &fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);

            // Create the texture
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
            glBindTexture(GL_TEXTURE_2D, 0);

            // Generate and bind the renderbuffer
            glGenRenderbuffers(1, &rbo);
            glBindRenderbuffer(GL_RENDERBUFFER, rbo);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, width, height);
            glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

            auto fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
                cerr << "FB_ERROR: " << fboStatus << endl;
            }

            // Unbind the framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        // Resize the FBO's texture
        void resize(int newWidth, int newHeight) {
            if (width == newWidth && height == newHeight)
                return;

            // Delete the old framebuffer and texture
            release();

            // Reinitialize with the new size
            init(newWidth, newHeight);
        }

        // Get the texture ID
        GLuint getTexture() const {
            return texture;
        }

        // Bind the framebuffer
        void bind() const {
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }

        // Unbind the framebuffer
        static void unbind() {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

    private:
        // Release resources
        void release() {
            if (texture) {
                glDeleteTextures(1, &texture);
                texture = 0;
            }

            if (fbo) {
                glDeleteFramebuffers(1, &fbo);
                fbo = 0;
            }
        }
};