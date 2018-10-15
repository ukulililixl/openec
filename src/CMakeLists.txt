# minimum version of cmake
cmake_minimum_required (VERSION 3.1)

# project name
project (openec_exe)

add_subdirectory(util)
add_subdirectory(common)

aux_source_directory(. DIR_SRCS)
add_executable(OECCoordinator OECCoordinator.cc)
add_executable(OECAgent OECAgent.cc)
add_executable(OECClient OECClient.cc)

target_link_libraries(OECCoordinator common pthread)
target_link_libraries(OECAgent common pthread)
target_link_libraries(OECClient common pthread)
