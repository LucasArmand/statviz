#pragma once

#include "app/Shader.hpp"

class Grid {
public:
    void init(float extent = 10.0f, float spacing = 1.0f);
    void draw(Shader& shader, const class Camera& camera);
    void cleanup();

private:
    unsigned int vao_ = 0, vbo_ = 0;
    int vertex_count_ = 0;
};
