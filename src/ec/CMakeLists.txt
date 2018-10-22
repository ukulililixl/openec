aux_source_directory(. DIR_LIB_SRCS)
add_library (ec ${DIR_LIB_SRCS})
target_link_libraries(ec gf_complete isal)
