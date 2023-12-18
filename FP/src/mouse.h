#pragma once

#include <glm/glm.hpp>

class Mouse {
    private:
        struct Position {
            double x;
            double y;
        };

    public:
        enum class CtrlDir : int {
            NORMAL = 1,
            REVERSED = -1,
        };

        CtrlDir ctrlDir = CtrlDir::NORMAL;
        Position lastCursorPos{0, 0};
        float xStep = 5.0f;
        float yStep = 5.0f;
        
        Mouse() {}
};