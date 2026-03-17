#include <gtest/gtest.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

// Camera logic duplicated here for pure CPU testing (no GL dependency)
struct TestCamera {
    float yaw = 0.4f, pitch = 0.6f, distance = 8.0f;
    glm::vec3 target{0.0f, 0.5f, 0.0f};
    int w = 1280, h = 720;
    float fov = 45.0f, near = 0.1f, far = 100.0f;

    glm::vec3 position() const {
        return target + glm::vec3(
            distance * std::sin(pitch) * std::cos(yaw),
            distance * std::cos(pitch),
            distance * std::sin(pitch) * std::sin(yaw)
        );
    }

    glm::mat4 view() const {
        return glm::lookAt(position(), target, glm::vec3(0, 1, 0));
    }

    glm::mat4 projection() const {
        return glm::perspective(glm::radians(fov), float(w) / float(h), near, far);
    }

    glm::mat4 vp() const { return projection() * view(); }
    glm::mat4 inv_vp() const { return glm::inverse(vp()); }
};

TEST(Camera, PositionIsAtCorrectDistance) {
    TestCamera cam;
    glm::vec3 pos = cam.position();
    float d = glm::length(pos - cam.target);
    EXPECT_NEAR(d, cam.distance, 1e-5f);
}

TEST(Camera, ViewProjectionInverseRoundtrip) {
    TestCamera cam;
    glm::mat4 vp = cam.vp();
    glm::mat4 inv = cam.inv_vp();
    glm::mat4 identity = vp * inv;

    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            EXPECT_NEAR(identity[i][j], (i == j) ? 1.0f : 0.0f, 1e-4f);
}

TEST(Camera, LookAtTargetIsInCenter) {
    TestCamera cam;
    glm::mat4 vp = cam.vp();
    glm::vec4 clip = vp * glm::vec4(cam.target, 1.0f);
    glm::vec3 ndc = glm::vec3(clip) / clip.w;
    // Target should be near center of screen (NDC ~0,0)
    EXPECT_NEAR(ndc.x, 0.0f, 0.15f);
    EXPECT_NEAR(ndc.y, 0.0f, 0.15f);
}

TEST(Camera, DifferentAspectRatios) {
    TestCamera cam;
    cam.w = 1920;
    cam.h = 1080;
    glm::mat4 proj = cam.projection();
    // Projection should be valid (non-zero determinant)
    EXPECT_GT(std::abs(glm::determinant(proj)), 1e-6f);
}
