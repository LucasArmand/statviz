find_package(GTest REQUIRED)

add_executable(statviz_tests
    test_camera.cpp
    test_sampling.cpp
    test_normalization.cpp
)

target_include_directories(statviz_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(statviz_tests PRIVATE
    GTest::gtest_main
    glm::glm
)

include(GoogleTest)
gtest_discover_tests(statviz_tests)
