cmake_minimum_required(VERSION 3.24)
project(cs6983_opengl_intro LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4
  GIT_SHALLOW TRUE
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 1.0.1
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glm)

add_executable(cs6983_opengl_intro
  src/main.cpp
  src/gl_loader.hpp
)

target_link_libraries(cs6983_opengl_intro PRIVATE glfw glm::glm)

# Forward slashes, so the path survives as a C string on Windows.
file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}/shaders" SHADER_DIR_FWD)
file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}/models" MODEL_DIR_FWD)
target_compile_definitions(cs6983_opengl_intro PRIVATE
  "SHADER_DIR=\"${SHADER_DIR_FWD}\""
  "MODEL_DIR=\"${MODEL_DIR_FWD}\"")

if (MSVC)
  target_compile_options(cs6983_opengl_intro PRIVATE /W4 /permissive-)
else()
  target_compile_options(cs6983_opengl_intro PRIVATE -Wall -Wextra -Wpedantic)
endif()
