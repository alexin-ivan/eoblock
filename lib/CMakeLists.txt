
if(CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${DEFAULT_CFLAGS}")
endif()

include_directories ("${PROJECT_SOURCE_DIR}/include")
add_library(oblock
    liboblock.c
    eob_log.c
    chksum.c
)
add_subdirectory(libavl) 
add_subdirectory(libvec) 
