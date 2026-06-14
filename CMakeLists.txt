cmake_minimum_required(VERSION 3.20)
project(mini-ecs VERSION 0.1.0 LANGUAGES CXX)

add_library(mini-ecs INTERFACE)
add_library(mini-ecs::mini-ecs ALIAS mini-ecs)

target_include_directories(mini-ecs INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

# The headers use C++17 (nested namespaces, structured bindings). Consumers
# linking against mini-ecs inherit this requirement automatically.
target_compile_features(mini-ecs INTERFACE cxx_std_17)

# Only build tests when mini-ecs is the top-level project, so submodule
# consumers (add_subdirectory) don't pay for them.
option(MINI_ECS_BUILD_TESTS "Build mini-ecs tests" OFF)
if(MINI_ECS_BUILD_TESTS AND CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    enable_testing()
    add_executable(mini-ecs-tests tests/test_basic.cpp)
    target_link_libraries(mini-ecs-tests PRIVATE mini-ecs)
    add_test(NAME mini-ecs-tests COMMAND mini-ecs-tests)
endif()
