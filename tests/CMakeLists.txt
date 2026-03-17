# Tests CMakeLists.txt
add_executable(ParticleGL_Tests
    test_main.cpp
    ecs/test_component_pool.cpp
    ecs/test_registry.cpp
    ecs/test_system.cpp
    ecs/test_components.cpp
    renderer/test_camera.cpp
    particles/test_particle_pool.cpp
    particles/test_particle_system.cpp
    serialization/test_component_serializer.cpp
    serialization/test_scene_serializer.cpp
    ../src/ecs/Registry.cpp
    ../src/renderer/Camera.cpp
    ../src/particles/ParticlePool.cpp
    ../src/ecs/systems/ParticleSystem.cpp
    ../src/serialization/SceneSerializer.cpp
    ../src/renderer/InstanceBuffer.cpp
)

target_link_libraries(ParticleGL_Tests PRIVATE GTest::gtest GTest::gtest_main glad nlohmann_json::nlohmann_json)
target_include_directories(ParticleGL_Tests PRIVATE ../src)
target_compile_definitions(ParticleGL_Tests PRIVATE PGL_TEST_ENV)

include(GoogleTest)
gtest_discover_tests(ParticleGL_Tests)
