#include "app/Renderer.hpp"
#include "app/Camera.hpp"

#include <glad/gl.h>
#include <spdlog/spdlog.h>

void Renderer::create_fullscreen_quad() {
    float quad[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f,  1.0f,
    };

    glGenVertexArrays(1, &quad_vao_);
    glGenBuffers(1, &quad_vbo_);

    glBindVertexArray(quad_vao_);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

bool Renderer::init(int width, int height) {
    width_ = width;
    height_ = height;

    create_fullscreen_quad();

    glGenTextures(1, &prob_tex_);
    glBindTexture(GL_TEXTURE_2D, prob_tex_);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, width, height);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glGenFramebuffers(1, &prob_fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, prob_fbo_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, prob_tex_, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        spdlog::error("Probability FBO incomplete");
        return false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenTextures(1, &output_tex_);
    glBindTexture(GL_TEXTURE_2D, output_tex_);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8UI, width, height);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glGenBuffers(1, &sum_ssbo_);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sum_ssbo_);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(unsigned int), nullptr, GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    if (!raymarch_shader_.load("shaders/fullscreen.vert", "shaders/raymarch.frag")) {
        spdlog::error("Failed to load ray march shader");
        return false;
    }
    if (!sample_shader_.load_compute("shaders/sample.comp")) {
        spdlog::error("Failed to load sample compute shader");
        return false;
    }
    if (!display_shader_.load("shaders/fullscreen.vert", "shaders/display.frag")) {
        spdlog::error("Failed to load display shader");
        return false;
    }

    return true;
}

void Renderer::render(const DistributionParams& params, const RenderConfig& config,
                      const Camera& camera, unsigned int frame) {
    // ── Pass 1: Ray march → probability texture ────────────────────────────
    glDisable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, prob_fbo_);
    glViewport(0, 0, width_, height_);
    glClear(GL_COLOR_BUFFER_BIT);

    raymarch_shader_.use();
    raymarch_shader_.set_mat4("u_inv_view_proj", camera.inv_view_projection());
    raymarch_shader_.set_vec3("u_camera_pos", camera.position());
    raymarch_shader_.set_vec3("u_mean", params.mean);
    raymarch_shader_.set_vec3("u_variance", params.variance);
    raymarch_shader_.set_float("u_step_size", config.step_size);
    raymarch_shader_.set_float("u_march_distance", config.march_distance);
    raymarch_shader_.set_int("u_kernel", static_cast<int>(params.kernel));

    glBindVertexArray(quad_vao_);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glEnable(GL_BLEND);

    glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);

    // ── Pass 2: Compute sampling ───────────────────────────────────────────
    unsigned int zero = 0;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sum_ssbo_);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(unsigned int), &zero);

    sample_shader_.use();
    sample_shader_.set_int("u_width", width_);
    sample_shader_.set_int("u_height", height_);
    sample_shader_.set_int("u_sample_count", config.sample_count);
    sample_shader_.set_uint("u_frame", frame);
    sample_shader_.set_int("u_pass", 0);

    glBindImageTexture(0, prob_tex_, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
    glBindImageTexture(1, output_tex_, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8UI);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, sum_ssbo_);

    int gx = (width_ + 15) / 16;
    int gy = (height_ + 15) / 16;
    glDispatchCompute(gx, gy, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    sample_shader_.set_int("u_pass", 1);
    glDispatchCompute(gx, gy, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // ── Pass 3: Display ────────────────────────────────────────────────────
    glViewport(0, 0, width_, height_);

    display_shader_.use();
    display_shader_.set_int("u_width", width_);
    display_shader_.set_int("u_height", height_);
    display_shader_.set_vec3("u_dot_color", config.dot_color);
    glBindImageTexture(0, output_tex_, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8UI);

    glBindVertexArray(quad_vao_);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void Renderer::cleanup() {
    if (quad_vbo_) { glDeleteBuffers(1, &quad_vbo_); quad_vbo_ = 0; }
    if (quad_vao_) { glDeleteVertexArrays(1, &quad_vao_); quad_vao_ = 0; }
    if (prob_fbo_) { glDeleteFramebuffers(1, &prob_fbo_); prob_fbo_ = 0; }
    if (prob_tex_) { glDeleteTextures(1, &prob_tex_); prob_tex_ = 0; }
    if (output_tex_) { glDeleteTextures(1, &output_tex_); output_tex_ = 0; }
    if (sum_ssbo_) { glDeleteBuffers(1, &sum_ssbo_); sum_ssbo_ = 0; }
}
