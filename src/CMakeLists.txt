# minimum version of cmake
cmake_minimum_required (VERSION 3.1)

# project name
project (openec_exe)

add_subdirectory(common)
add_subdirectory(ec)
add_subdirectory(fs)
add_subdirectory(protocol)
add_subdirectory(util)

aux_source_directory(. DIR_SRCS)
add_executable(OECCoordinator OECCoordinator.cc)
add_executable(OECAgent OECAgent.cc)
add_executable(OECClient OECClient.cc)
add_executable(ECDAGTest ECDAGTest.cc)
add_executable(CodeTest CodeTest.cc)

if (${FS_TYPE} MATCHES "HDFS")
  add_executable(HDFSClient HDFSClient.cc)
endif(${FS_TYPE} MATCHES "HDFS")

target_link_libraries(OECCoordinator common pthread fs)
target_link_libraries(OECAgent common pthread fs)
target_link_libraries(OECClient common pthread)
target_link_libraries(ECDAGTest common ec)
target_link_libraries(CodeTest common ec)

if (${FS_TYPE} MATCHES "HDFS")
  target_link_libraries(HDFSClient common fs)
endif(${FS_TYPE} MATCHES "HDFS")

