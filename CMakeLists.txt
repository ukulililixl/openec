# minimum version of cmake
cmake_minimum_required (VERSION 3.1)

# project name
project (openec)

set (CMAKE_CXX_STANDARD 11)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -mmmx -msse -mpclmul -msse4.2 -DINTEL_SSE4 -mavx")

if (${FS_TYPE} MATCHES "HDFS3")
  add_definitions(-DHDFS3)
endif(${FS_TYPE} MATCHES "HDFS3")

if (${FS_TYPE} MATCHES "HDFSRAID")
  add_definitions(-DHDFSRAID)
endif(${FS_TYPE} MATCHES "HDFSRAID")

if (${FS_TYPE} MATCHES "QFS")
  add_definitions(-DQFS)
endif(${FS_TYPE} MATCHES "QFS")

# subdirectory
add_subdirectory(src)
