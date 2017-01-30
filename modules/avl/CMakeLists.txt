foreach(FNAME ${MODULE_SOURCE_FILES})
endforeach(FNAME)

set(MODULE_OUTPUT_FILES
	avl.o
	avl.mod.c
	avl.mod.o
	avl.ko

	.avl.o.cmd
	.avl.ko.cmd
	.avl.mod.o.cmd
	
	.tmp_versions/avl.mod
	.tmp_versions/ 
	
	modules.order
)


set(MODULE_SOURCE_FILES ${PROJECT_SOURCE_DIR}/lib/libavl/avl.c)

set(MODULE_FILENAME avl.ko)
set(MODULE_TARGET_NAMME avl-kmod)
set(MODULE_SOURCE_DIR  ${CMAKE_CURRENT_BINARY_DIR})
set(KERNEL_INCLUDES "${PROJECT_SOURCE_DIR}/include" "${PROJECT_SOURCE_DIR}/include/sys")

set(KBUILD_EXTRA_CFLAGS 
    -g -I${KERNEL_INCLUDES} -DEOB_KERNEL
)

set(KERNEL_DIR ${KERNEL_BUILD_DIR} )
set(KBUILD_CMD ${CMAKE_MAKE_PROGRAM} -C ${KERNEL_DIR} M=${MODULE_SOURCE_DIR} EXTRA_CFLAGS="${KBUILD_EXTRA_CFLAGS}" obj-m=avl.o)
set(KBUILD_CMD_CLEAN ${CMAKE_MAKE_PROGRAM} -C ${KERNEL_DIR} M=${MODULE_SOURCE_DIR} EXTRA_CFLAGS="${KBUILD_EXTRA_CFLAGS}" clean obj-m=avl.o)

set(KMOD ${CMAKE_CURRENT_BINARY_DIR}/avl.ko)

if (USE_KMOD)
	add_custom_command(
	    OUTPUT ${MODULE_OUTPUT_FILES}
	    COMMAND ${CMAKE_COMMAND} -E create_symlink ${MODULE_SOURCE_FILES} ${CMAKE_CURRENT_BINARY_DIR}/avl.c
	    COMMAND ${KBUILD_CMD}
	    MAIN_DEPENDENCY ${MODULE_SOURCE_FILES}
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Eoblock kernel module build"
	)

	add_custom_target(avl-kmod-clean)
	add_custom_command(
	    TARGET avl-kmod-clean
	    COMMAND ${KBUILD_CMD_CLEAN}
	    COMMAND rm -fR ${CMAKE_CURRENT_BINARY_DIR}/avl.c
		MAIN_DEPENDENCY clean
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Eoblock kernel module clean"
	)

	foreach(GFILE ${MODULE_OUTPUT_FILES})
	    message(STATUS "File to remove: ${CMAKE_CURRENT_BINARY_DIR}/${GFILE}")
	    set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/${GFILE})
	endforeach(GFILE)

	set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/avl.c)
	add_custom_target(avl-kmod-build ALL DEPENDS ${MODULE_OUTPUT_FILES})

	install(FILES ${KMOD} DESTINATION /lib/modules/${KERNEL_RELEASE}/updates/)
endif(USE_KMOD)
