#include "app/UI.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

void UI::init(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 450");
    ImGui::StyleColorsDark();
}

void UI::begin_frame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UI::draw(DistributionParams& params, RenderConfig& config, float fps) {
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(320, 360), ImGuiCond_FirstUseEver);
    ImGui::Begin("Controls");

    ImGui::Text("FPS: %.1f", fps);
    ImGui::Text("WASD: move | Mouse drag: orbit | Scroll: zoom");
    ImGui::Separator();

    // Kernel selector
    int kernel = static_cast<int>(params.kernel);
    if (ImGui::Combo("Distribution", &kernel, kernel_names,
                     static_cast<int>(Kernel::Count))) {
        params.kernel = static_cast<Kernel>(kernel);
    }

    ImGui::Separator();
    // Logarithmic slider: 1 to 1,000,000
    float log_n = log10f(static_cast<float>(config.sample_count));
    if (ImGui::SliderFloat("Sample Count (N)", &log_n, 0.0f, 6.0f, "%.2f")) {
        config.sample_count = static_cast<int>(powf(10.0f, log_n));
        if (config.sample_count < 1) config.sample_count = 1;
    }
    ImGui::SameLine();
    ImGui::Text("%d", config.sample_count);
    ImGui::SliderFloat("Step Size", &config.step_size, 0.01f, 0.2f);
    ImGui::SliderFloat("March Distance", &config.march_distance, 2.0f, 200.0f);

    ImGui::Separator();
    ImGui::Text("Parameters");
    ImGui::SliderFloat3("Mean", &params.mean.x, -10.0f, 10.0f);
    ImGui::SliderFloat3("Variance", &params.variance.x, 0.1f, 10.0f);

    ImGui::Separator();
    ImGui::ColorEdit3("Dot Color", &config.dot_color.x);

    ImGui::End();
}

void UI::end_frame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UI::shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
