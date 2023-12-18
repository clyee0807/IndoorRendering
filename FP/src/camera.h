#pragma once

#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
    public:
        bool enableMotion = true;
        float zoomStep = 0.05f;

        // Spherical Coordinates
        float distance = 1.0f;

        // Position
        
        glm::vec3 focus{ 0.0f };
        glm::vec3 upVec{0.0f, 1.0f, 0.0f}, forwardVec{0.0f}, rightVec{0.0f};
        Camera(float initDistance, float initTheta, float initPhi, const glm::vec3& initFocus,
               float initFoV, float initAP, float initNear, float initFar, const glm::vec3& initUpVec)
            : distance(initDistance), theta(initTheta), phi(initPhi), focus(initFocus),
              FoV(initFoV), aspectRatio(initAP), zNear(initNear), zFar(initFar), upVec(initUpVec) {
            // Update with initial values
            updatePosition();
        }

    private:
        const float DEG_EYE = 90.0f;
        const float MIN_DISTANCE = 0.05f;
        const float MAX_DISTANCE = 2.0f;

        // View
        glm::vec3 position{ 0.0f };
        float dampedDistance = 0.0f;
        float theta = 90.0f;
        float phi = (-90.0f);

        // Projection
        float FoV;
        float aspectRatio;
        float zNear;
        float zFar;

        void updateDirVec() {
            forwardVec = getDirection();
            rightVec = cross(forwardVec, upVec);
            forwardVec.y = 0.0f;
            rightVec.y = 0.0f;
            forwardVec = normalize(forwardVec);
            rightVec = normalize(rightVec);
        }

        float mapValue(float value, float start1, float stop1, float start2, float stop2) {
            return start2 + (stop2 - start2) * ((value - start1) / (stop1 - start1));
        }
        
    public:
        // TODO: Optional TIME variable
        void updatePosition() {
            float zeta = theta > DEG_EYE ? pow(cos(glm::radians(theta - DEG_EYE)), 2) : 1;
            dampedDistance = zeta * distance;

            float thetaR = glm::radians(theta);
            float phiR = glm::radians(phi);
            position = focus + glm::vec3(dampedDistance * sin(thetaR) * cos(phiR),
                                         dampedDistance * cos(thetaR),
                                         dampedDistance * sin(thetaR) * sin(phiR));
            updateDirVec();
        }

        void zoomIn() {
            distance = std::max(distance - zoomStep, MIN_DISTANCE);
        }

        void zoomOut() {
            distance = std::min(distance + zoomStep, MAX_DISTANCE);
        }

        glm::mat4 projMatrix() const {
            return glm::perspective(FoV, aspectRatio, zNear, zFar);
        }

        glm::mat4 viewMatrix() const {
            return glm::lookAt(position, focus, upVec);
        }

        glm::vec3 getDirection() const {
            return normalize(focus - position);
        }

        glm::vec3 getPosition() const {
            return position;
        }

        float getRealDistance() const {
            return dampedDistance;
        }

        float getTheta() const {
            return theta;
        }

        float getPhi() const {
            return phi;
        }

        void setTheta(float theta) {
            this->theta = theta;
        }

        void setPhi(float phi) {
            this -> phi = phi;
        }

        float getNear() const {
            return zNear;
        }

        float getFar() const {
            return zFar;
        }

        glm::vec3 getUpVector() const {
            return upVec;
        }
};