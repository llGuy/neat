cmake_minimum_required(VERSION "3.2")

project(neuroevolution)

set (CMAKE_EXPORT_COMPILE_COMMANDS ON)
set (CMAKE_CXX_STANDARD 11)

file(GLOB_RECURSE SOURCES "source/*.cpp")

add_executable(neuroevolution "${SOURCES}")
