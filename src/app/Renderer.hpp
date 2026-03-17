#pragma once

#include "app/Shader.hpp"
#include "common/Config.hpp"

class Renderer {
public:
    bool init(int width, int height);
    void render(const DistributionParams& params, const RenderConfig& config,
                const class Camera& camera, unsigned int frame);
    void cleanup();

private:
    void create_fullscreen_quad();

    int width_ = 0, height_ = 0;
    unsigned int quad_vao_ = 0, quad_vbo_ = 0;

    // Ray march pass
    unsigned int prob_tex_ = 0;
    unsigned int prob_fbo_ = 0;
    Shader raymarch_shader_;

    // Compute sampling pass
    unsigned int output_tex_ = 0;
    unsigned int sum_ssbo_ = 0;
    Shader sample_shader_;

    // Display pass
    Shader display_shader_;
};
