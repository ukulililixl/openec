aux_source_directory(. DIR_LIB_SRCS)
add_library (util ${DIR_LIB_SRCS})
target_link_libraries(util gf_complete hiredis)
