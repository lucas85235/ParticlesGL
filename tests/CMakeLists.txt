# Tests CMakeLists.txt
add_executable(ParticleGL_Tests
    test_main.cpp
)

target_link_libraries(ParticleGL_Tests PRIVATE GTest::gtest GTest::gtest_main)
target_include_directories(ParticleGL_Tests PRIVATE ../src)

include(GoogleTest)
gtest_discover_tests(ParticleGL_Tests)
