# Tests CMakeLists.txt
add_executable(ParticleGL_Tests
    test_main.cpp
    ecs/test_component_pool.cpp
    ecs/test_registry.cpp
    ecs/test_system.cpp
    ecs/test_components.cpp
    renderer/test_camera.cpp
    ../src/ecs/Registry.cpp
    ../src/renderer/Camera.cpp
)

target_link_libraries(ParticleGL_Tests PRIVATE GTest::gtest GTest::gtest_main)
target_include_directories(ParticleGL_Tests PRIVATE ../src)

include(GoogleTest)
gtest_discover_tests(ParticleGL_Tests)
