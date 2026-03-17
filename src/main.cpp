#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "app/Shader.hpp"
#include "app/Camera.hpp"
#include "app/Renderer.hpp"
#include "app/Grid.hpp"
#include "app/UI.hpp"
#include "common/Config.hpp"

#include <imgui.h>
#include <spdlog/spdlog.h>
#include <chrono>

// ── Input state ─────────────────────────────────────────────────────────────

struct InputState {
    Camera* camera = nullptr;
    bool dragging = false;
    double last_mx = 0, last_my = 0;
};

static void mouse_button_callback(GLFWwindow* w, int button, int action, int /*mods*/) {
    auto* state = static_cast<InputState*>(glfwGetWindowUserPointer(w));
    if (!state) return;
    if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            state->dragging = true;
            glfwGetCursorPos(w, &state->last_mx, &state->last_my);
        } else if (action == GLFW_RELEASE) {
            state->dragging = false;
        }
    }
}

static void cursor_pos_callback(GLFWwindow* w, double xpos, double ypos) {
    auto* state = static_cast<InputState*>(glfwGetWindowUserPointer(w));
    if (!state || !state->dragging) return;
    if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse) return;

    float dx = static_cast<float>(xpos - state->last_mx);
    float dy = static_cast<float>(ypos - state->last_my);
    state->last_mx = xpos;
    state->last_my = ypos;

    state->camera->orbit(dx, dy);
}

static void scroll_callback(GLFWwindow* w, double /*xoff*/, double yoff) {
    auto* state = static_cast<InputState*>(glfwGetWindowUserPointer(w));
    if (!state) return;
    if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse) return;

    state->camera->zoom(static_cast<float>(yoff));
}

static void key_callback(GLFWwindow* w, int key, int /*scancode*/, int action, int /*mods*/) {
    if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureKeyboard) return;
    if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE)
        glfwSetWindowShouldClose(w, GLFW_TRUE);
}

static void process_wasd(GLFWwindow* window, Camera& camera, float dt) {
    if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureKeyboard) return;

    float forward = 0.0f, right = 0.0f;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) forward += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) forward -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) right += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) right -= 1.0f;

    if (forward != 0.0f || right != 0.0f)
        camera.move(forward, right, dt);
}

// ── Main ────────────────────────────────────────────────────────────────────

int main() {
    RenderConfig config;
    DistributionParams params;

    if (!glfwInit()) {
        spdlog::critical("Failed to initialize GLFW");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(config.window_width, config.window_height,
                                           "statviz", nullptr, nullptr);
    if (!window) {
        spdlog::critical("Failed to create GLFW window");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    int version = gladLoadGL(glfwGetProcAddress);
    if (!version) {
        spdlog::critical("Failed to initialize GLAD");
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    spdlog::info("OpenGL {}.{}", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));

    Camera camera(config.window_width, config.window_height);

    Renderer renderer;
    if (!renderer.init(config.window_width, config.window_height)) {
        spdlog::critical("Failed to initialize renderer");
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    Grid grid;
    grid.init();

    Shader grid_shader;
    if (!grid_shader.load("shaders/grid.vert", "shaders/grid.frag")) {
        spdlog::warn("Failed to load grid shaders");
    }

    InputState input;
    input.camera = &camera;
    glfwSetWindowUserPointer(window, &input);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    UI ui;
    ui.init(window);

    auto prev_time = std::chrono::high_resolution_clock::now();
    float fps = 0.0f;
    float fps_timer = 0.0f;
    int frame_count = 0;
    unsigned int frame_number = 0;

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        auto now = std::chrono::high_resolution_clock::now();
        float wall_dt = std::chrono::duration<float>(now - prev_time).count();
        prev_time = now;
        if (wall_dt > 0.1f) wall_dt = 0.1f;

        // WASD camera movement
        process_wasd(window, camera, wall_dt);

        fps_timer += wall_dt;
        frame_count++;
        if (fps_timer >= 1.0f) {
            fps = frame_count / fps_timer;
            fps_timer = 0.0f;
            frame_count = 0;
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderer.render(params, config, camera, frame_number);

        if (grid_shader.id() != 0) {
            glEnable(GL_DEPTH_TEST);
            grid.draw(grid_shader, camera);
            glDisable(GL_DEPTH_TEST);
        }

        ui.begin_frame();
        ui.draw(params, config, fps);
        ui.end_frame();

        glfwSwapBuffers(window);
        frame_number++;
    }

    ui.shutdown();
    grid.cleanup();
    renderer.cleanup();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
