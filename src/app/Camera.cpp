#include "app/Camera.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

Camera::Camera(int screen_w, int screen_h)
    : screen_w_(screen_w), screen_h_(screen_h) {}

void Camera::orbit(float dx, float dy) {
    yaw_ += dx * 0.005f;
    pitch_ += dy * 0.005f;
    pitch_ = std::clamp(pitch_, 0.1f, 3.04f);
}

void Camera::zoom(float delta) {
    distance_ *= (delta > 0) ? 0.9f : 1.1f;
    distance_ = std::clamp(distance_, 0.5f, 100.0f);
}

void Camera::move(float forward, float right, float dt) {
    // Forward direction is from camera toward target, projected onto XZ
    glm::vec3 to_target = target_ - position();
    glm::vec3 fwd = glm::normalize(glm::vec3(to_target.x, 0.0f, to_target.z));
    glm::vec3 rgt = glm::normalize(glm::cross(fwd, glm::vec3(0.0f, 1.0f, 0.0f)));

    glm::vec3 delta_pos = (fwd * forward + rgt * right) * move_speed_ * dt;
    target_ += delta_pos;
}

void Camera::resize(int w, int h) {
    screen_w_ = w;
    screen_h_ = h;
}

glm::vec3 Camera::position() const {
    return target_ + glm::vec3(
        distance_ * std::sin(pitch_) * std::cos(yaw_),
        distance_ * std::cos(pitch_),
        distance_ * std::sin(pitch_) * std::sin(yaw_)
    );
}

glm::mat4 Camera::view() const {
    return glm::lookAt(position(), target_, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 Camera::projection() const {
    float aspect = static_cast<float>(screen_w_) / static_cast<float>(screen_h_);
    return glm::perspective(glm::radians(fov_), aspect, near_, far_);
}

glm::mat4 Camera::view_projection() const {
    return projection() * view();
}

glm::mat4 Camera::inv_view_projection() const {
    return glm::inverse(view_projection());
}
