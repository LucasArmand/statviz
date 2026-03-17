#pragma once

#include "common/Config.hpp"

struct GLFWwindow;

class UI {
public:
    void init(GLFWwindow* window);
    void begin_frame();
    void draw(DistributionParams& params, RenderConfig& config, float fps);
    void end_frame();
    void shutdown();
};
