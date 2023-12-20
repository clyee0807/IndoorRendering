#pragma once

enum class FBOType {
    FBOType_Normal = 0,
    FBOType_Depth  = 1,
};

class FBO {
    private:
        GLuint fbo, rbo, texture;
        int width, height;
        FBOType type;

    public:
        FBO(int w, int h, FBOType t) : fbo(0), rbo(0), texture(0), width(w), height(h), type(t) {
            // You cannot use OpenGL functions before initializing...
            // init(width, height, type);
        }

        ~FBO() {
            release();
        }

        void init() {
            init(this->width, this->height, this->type);
        }

        void init(int w, int h, FBOType t) {
            // Ensure it updates all parameters everytime
            this->width = w;
            this->height = h;
            this->type = t;

            // Framebuffer
            glGenFramebuffers(1, &fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);

            // Create texture
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);

            if (t == FBOType::FBOType_Normal) {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
                glBindTexture(GL_TEXTURE_2D, 0);

                // Renderbuffer
                glGenRenderbuffers(1, &rbo);
                glBindRenderbuffer(GL_RENDERBUFFER, rbo);
                glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, w, h);
                glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);
            } else if (t == FBOType::FBOType_Depth) {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                float clampColor[4] = { 1.0f };
                glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, clampColor);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture, 0);

                glDrawBuffer(GL_NONE);
                glReadBuffer(GL_NONE);
            }

            auto fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
                cerr << "FB_ERROR: " << fboStatus << endl;
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
            init(newWidth, newHeight, this->type);
        }

        GLuint getTexture() const {
            return texture;
        }

        void bind() const {
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }

        static void unbind() {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

    private:
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