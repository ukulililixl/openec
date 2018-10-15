
# minimum version of cmake
cmake_minimum_required (VERSION 3.1)

# project name
project (openec)

set (CMAKE_CXX_STANDARD 11)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# subdirectory
add_subdirectory(src)
