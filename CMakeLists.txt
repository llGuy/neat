cmake_minimum_required(VERSION "3.2")

project(neuroevolution)

set (CMAKE_EXPORT_COMPILE_COMMANDS ON)
set (CMAKE_CXX_STANDARD 11)

file(GLOB_RECURSE SOURCES "source/*.cpp")

find_package(OpenGL REQUIRED)
if (OPENGL_FOUND)
  link_libraries("${OPENGL_LIBRARIES}")
endif()

find_package(GLEW REQUIRED)
if (GLEW_FOUND)
  link_libraries("${GLEW_LIBRARIES}")
endif()

link_libraries("glfw")

add_executable(neuroevolution "${SOURCES}")
