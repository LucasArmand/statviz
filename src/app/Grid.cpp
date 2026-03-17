#include "app/Grid.hpp"
#include "app/Camera.hpp"

#include <glad/gl.h>
#include <vector>

void Grid::init(float extent, float spacing) {
    std::vector<float> verts;

    for (float x = -extent; x <= extent; x += spacing) {
        // Line along Z
        verts.push_back(x); verts.push_back(0.0f); verts.push_back(-extent);
        verts.push_back(x); verts.push_back(0.0f); verts.push_back(extent);
    }
    for (float z = -extent; z <= extent; z += spacing) {
        // Line along X
        verts.push_back(-extent); verts.push_back(0.0f); verts.push_back(z);
        verts.push_back(extent);  verts.push_back(0.0f); verts.push_back(z);
    }

    vertex_count_ = static_cast<int>(verts.size() / 3);

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(verts.size() * sizeof(float)),
                 verts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void Grid::draw(Shader& shader, const Camera& camera) {
    shader.use();
    shader.set_mat4("u_mvp", camera.view_projection());

    glBindVertexArray(vao_);
    glDrawArrays(GL_LINES, 0, vertex_count_);
    glBindVertexArray(0);
}

void Grid::cleanup() {
    if (vbo_) { glDeleteBuffers(1, &vbo_); vbo_ = 0; }
    if (vao_) { glDeleteVertexArrays(1, &vao_); vao_ = 0; }
}
