include_directories("${PROJECT_SOURCE_DIR}/include")
add_executable(
	iface0 
	${CMAKE_CURRENT_SOURCE_DIR}/iface.c
	${CMAKE_CURRENT_SOURCE_DIR}/iface_test.c
)
target_link_libraries(iface0 oblock avl z udev)

if(CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${DEFAULT_CFLAGS}")
endif()

add_custom_target(iface0-run DEPENDS iface0)
add_custom_command(
    TARGET iface0-run
    COMMAND gdb --args ${CMAKE_CURRENT_BINARY_DIR}/iface0
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Run iface0"
)

add_custom_target(iface0-run1 DEPENDS iface0)
add_custom_command(
    TARGET iface0-run1
    COMMAND gdb --args ${CMAKE_CURRENT_BINARY_DIR}/iface0 test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Run iface0"
)

install(TARGETS iface0 DESTINATION bin)
