set(MODULE_SOURCE_FILES
    eoblock.c
)

foreach(FNAME ${MODULE_SOURCE_FILES})
endforeach(FNAME)

set(MODULE_OUTPUT_FILES
	
	eoblock.o
	eoblock.mod.c
	eoblock.mod.o
	eoblock.ko

	.eoblock.o.cmd
	.eoblock.ko.cmd
	.eoblock.mod.o.cmd
	
	.tmp_versions/eoblock.mod
	.tmp_versions/ 
	
	modules.order
)


set(MODULE_SOURCE_FILES eoblock.c)

set(MODULE_FILENAME oeblock.ko)
set(MODULE_TARGET_NAMME eoblock-kmod)
set(MODULE_SOURCE_DIR  ${CMAKE_CURRENT_BINARY_DIR})
set(KERNEL_INCLUDES "${PROJECT_SOURCE_DIR}/include")

set(KBUILD_EXTRA_CFLAGS 
    -g -I${KERNEL_INCLUDES} -DEOB_KERNEL
)

set(KERNEL_DIR ${KERNEL_BUILD_DIR} )
set(KBUILD_CMD ${CMAKE_MAKE_PROGRAM} -C ${KERNEL_DIR} M=${MODULE_SOURCE_DIR} EXTRA_CFLAGS="${KBUILD_EXTRA_CFLAGS}" obj-m=eoblock.o)
set(KBUILD_CMD_CLEAN ${CMAKE_MAKE_PROGRAM} -C ${KERNEL_DIR} M=${MODULE_SOURCE_DIR} EXTRA_CFLAGS="${KBUILD_EXTRA_CFLAGS}" clean obj-m=eoblock.o)

set(KMOD ${CMAKE_CURRENT_BINARY_DIR}/eoblock.ko)

if (USE_KMOD)
	add_custom_command(
	    OUTPUT ${MODULE_OUTPUT_FILES}
	    COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/eoblock.c ${CMAKE_CURRENT_BINARY_DIR}/eoblock.c
	    COMMAND ${KBUILD_CMD}
		MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/eoblock.c
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Eoblock kernel module build"
	)

	add_custom_target(kmod-clean)
	add_custom_command(
	    TARGET kmod-clean
	    COMMAND ${KBUILD_CMD_CLEAN}
	    COMMAND rm -fR ${CMAKE_CURRENT_BINARY_DIR}/eoblock.c
		MAIN_DEPENDENCY clean
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Eoblock kernel module clean"
	)

	foreach(GFILE ${MODULE_OUTPUT_FILES})
	    message(STATUS "File to remove: ${CMAKE_CURRENT_BINARY_DIR}/${GFILE}")
	    set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/${GFILE})
	endforeach(GFILE)

	set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/eoblock.c)
	add_custom_target(kmod-build ALL DEPENDS ${MODULE_OUTPUT_FILES})

	install(FILES ${KMOD} DESTINATION /lib/modules/${KERNEL_RELEASE}/updates/)
endif(USE_KMOD)
