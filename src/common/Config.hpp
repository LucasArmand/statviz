#pragma once

#include <glm/glm.hpp>

enum class Kernel : int {
    Gaussian = 0,
    BimodalGaussian,
    TrimodalGaussian,
    Torus,
    Orbital_2p,
    Orbital_3dz2,
    Orbital_3dxy,
    Count
};

inline const char* kernel_names[] = {
    "Gaussian",
    "Bimodal Gaussian",
    "Trimodal Gaussian",
    "Torus",
    "Orbital 2p (dumbbell)",
    "Orbital 3d_z2",
    "Orbital 3d_xy",
};

struct DistributionParams {
    glm::vec3 mean{0.0f, 1.0f, 0.0f};
    glm::vec3 variance{1.0f, 1.0f, 1.0f};
    Kernel kernel = Kernel::Gaussian;
};

struct RenderConfig {
    int sample_count = 5000;
    float step_size = 0.05f;
    float march_distance = 50.0f;
    glm::vec3 dot_color{0.4f, 0.8f, 1.0f};
    int window_width = 2560;
    int window_height = 1440;
};
