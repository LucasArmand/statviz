cmake_minimum_required(VERSION 3.25)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "Vcpkg toolchain file")

project(statviz LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Dependencies ─────────────────────────────────────────────────────────────
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(spdlog REQUIRED)
find_package(GTest REQUIRED)

# ── GLAD v2 via FetchContent ────────────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG        v2.0.8
    SOURCE_SUBDIR  cmake
)
FetchContent_MakeAvailable(glad)
glad_add_library(glad_gl45 REPRODUCIBLE API gl:core=4.5)

# ── GLM via FetchContent ────────────────────────────────────────────────────
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
)
FetchContent_MakeAvailable(glm)

# ── ImGui via FetchContent ──────────────────────────────────────────────────
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.8
)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PUBLIC glfw OpenGL::GL)

# ── Application library ────────────────────────────────────────────────────
file(GLOB APP_SOURCES ${CMAKE_SOURCE_DIR}/src/app/*.cpp)

if(APP_SOURCES)
    add_library(statviz_app STATIC ${APP_SOURCES})
    target_include_directories(statviz_app PUBLIC ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(statviz_app PUBLIC
        glad_gl45
        glfw
        OpenGL::GL
        glm::glm
        imgui
        spdlog::spdlog
    )
else()
    add_library(statviz_app INTERFACE)
    target_include_directories(statviz_app INTERFACE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(statviz_app INTERFACE
        glad_gl45
        glfw
        OpenGL::GL
        glm::glm
        imgui
        spdlog::spdlog
    )
    message(STATUS "No app sources found yet — statviz_app is INTERFACE only")
endif()

# ── Main executable ────────────────────────────────────────────────────────
if(EXISTS ${CMAKE_SOURCE_DIR}/src/main.cpp)
    add_executable(statviz src/main.cpp)
    target_include_directories(statviz PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(statviz PRIVATE statviz_app)

    # ── Shader copying ──────────────────────────────────────────────────────
    file(GLOB SHADER_SOURCES ${CMAKE_SOURCE_DIR}/src/shaders/*)

    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/.shaders.stamp
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/src/shaders
            ${CMAKE_CURRENT_BINARY_DIR}/shaders
        COMMAND ${CMAKE_COMMAND} -E touch
            ${CMAKE_CURRENT_BINARY_DIR}/.shaders.stamp
        DEPENDS ${SHADER_SOURCES}
        COMMENT "Copying shaders to build directory"
    )

    add_custom_target(copy_shaders ALL
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/.shaders.stamp
    )
    add_dependencies(statviz copy_shaders)
else()
    message(STATUS "src/main.cpp not found yet — skipping statviz executable")
endif()

# ── Tests ───────────────────────────────────────────────────────────────────
option(BUILD_TESTING "Build unit tests" ON)
if(BUILD_TESTING AND EXISTS ${CMAKE_SOURCE_DIR}/tests/CMakeLists.txt)
    enable_testing()
    add_subdirectory(tests)
endif()
