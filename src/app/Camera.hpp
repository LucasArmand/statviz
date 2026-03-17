#pragma once

#include <glm/glm.hpp>

class Camera {
public:
    Camera(int screen_w, int screen_h);

    void orbit(float dx, float dy);
    void zoom(float delta);
    void move(float forward, float right, float dt);
    void resize(int w, int h);

    glm::mat4 view() const;
    glm::mat4 projection() const;
    glm::mat4 view_projection() const;
    glm::mat4 inv_view_projection() const;
    glm::vec3 position() const;

    float yaw() const { return yaw_; }
    float pitch() const { return pitch_; }
    float distance() const { return distance_; }

private:
    float yaw_ = 0.4f;
    float pitch_ = 0.6f;
    float distance_ = 8.0f;
    glm::vec3 target_{0.0f, 0.5f, 0.0f};
    int screen_w_, screen_h_;
    float fov_ = 45.0f;
    float near_ = 0.1f;
    float far_ = 200.0f;
    float move_speed_ = 5.0f;
};
