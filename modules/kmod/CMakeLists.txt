cmake_minimum_required(VERSION 2.6)
# Module name
set(MODULE_NAME eoblock)

# Main file name
set(MODULE_SOURCE_FILES 
    ${MODULE_NAME}_main.c
)
# Source files
#set(MODULE_INPUT_FILES
#    eoblock_disk.c
#)
file(GLOB c_files
    "*.c"
)

foreach(GFILE ${c_files})
    get_filename_component(fname ${GFILE} NAME)
    message(STATUS "Added source file: ${fname}")
    list(APPEND MODULE_INPUT_FILES "${fname}")
endforeach(GFILE)

file(GLOB h_files
    "*.h"
)

set(MODULE_INPUT_HEADERS "")

foreach(GFILE ${h_files})
    get_filename_component(fname ${GFILE} NAME)
    message(STATUS "Added header file: ${fname}")
    list(APPEND MODULE_INPUT_HEADERS "${fname}")
endforeach(GFILE)

set(MODULE_OBJECT
    ${MODULE_NAME}_main.o
)

# Target module name
set(MODULE_FILENAME ${MODULE_NAME}.ko)
set(MODULE_BIN_FILENAME ${CMAKE_CURRENT_BINARY_DIR}/${MODULE_FILENAME})

# Name of module target
set(MODULE_TARGET_NAME eoblock-kmod-build)

set(MODULE_CLEAN_TARGET_NAME eoblock-kmod-clean)

set(MODULE_OUTPUT_FILES
    Module.symvers
    built-in.o
    .built-in.o.cmd

    ${MODULE_OBJECT}

    ${MODULE_NAME}.mod.c
    ${MODULE_NAME}.mod.o

    ${MODULE_FILENAME}

    .${MODULE_OBJECT}.cmd
    .${MODULE_FILENAME}.cmd
    .${MODULE_NAME}.mod.o.cmd
    
    .tmp_versions/${MODULE_NAME}.mod
    .tmp_versions/ 
    
    modules.order
)


set(MODULE_SOURCE_DIR  ${CMAKE_CURRENT_BINARY_DIR})
set(KERNEL_INCLUDES "${PROJECT_SOURCE_DIR}/include")

set(KBUILD_EXTRA_CFLAGS 
    -g -I${KERNEL_INCLUDES} -DEOB_KERNEL
)

set(MODULE_OBJECTS "")

foreach(GFILE ${MODULE_INPUT_FILES})
    get_filename_component(name_without_extension ${GFILE} NAME_WE)
    message(STATUS "Added object file: ${CMAKE_CURRENT_BINARY_DIR}/${name_without_extension}.o")
    list(APPEND MODULE_OBJECTS "${name_without_extension}.o")
    list(APPEND MODULE_OUTPUT_FILES "${name_without_extension}.o")
    list(APPEND MODULE_OUTPUT_FILES ".${name_without_extension}.o.cmd")
endforeach(GFILE)

#foreach(GFILE ${MODULE_OBJECTS})
    #list(APPEND MODULE_OUTPUT_FILES "${CMAKE_CURRENT_BINARY_DIR}/${GFILE}")
#ENDFOREACH(f)

set(KERNEL_DIR ${KERNEL_BUILD_DIR} )
set(KERNEL_CMD_ARGS obj-m+=${MODULE_NAME}.o ${MODULE_NAME}-objs="${MODULE_OBJECTS}")
set(KBUILD_CMD ${CMAKE_MAKE_PROGRAM} -C ${KERNEL_DIR} M=${MODULE_SOURCE_DIR} EXTRA_CFLAGS="${KBUILD_EXTRA_CFLAGS}" "${KERNEL_CMD_ARGS}")
set(KBUILD_CMD_CLEAN ${CMAKE_MAKE_PROGRAM} -C ${KERNEL_DIR} M=${MODULE_SOURCE_DIR} EXTRA_CFLAGS="${KBUILD_EXTRA_CFLAGS}" clean "${KERNEL_CMD_ARGS}")


if (USE_KMOD)
    set(MODULE_SOURCE_LINKS "")

    foreach(GFILE ${MODULE_INPUT_FILES})
        list(APPEND MODULE_SOURCE_LINKS ${CMAKE_CURRENT_BINARY_DIR}/${GFILE})
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${GFILE}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/${GFILE} ${CMAKE_CURRENT_BINARY_DIR}/${GFILE}
            MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/${GFILE}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Copy source ${GFILE}"
        )
    endforeach(GFILE)

    foreach(GFILE ${MODULE_INPUT_HEADERS})
        list(APPEND MODULE_SOURCE_LINKS ${CMAKE_CURRENT_BINARY_DIR}/${GFILE})
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${GFILE}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/${GFILE} ${CMAKE_CURRENT_BINARY_DIR}/${GFILE}
            MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/${GFILE}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Copy header ${GFILE}"
        )
    endforeach(GFILE)
    
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${MODULE_SOURCE_FILES}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/${MODULE_SOURCE_FILES} ${CMAKE_CURRENT_BINARY_DIR}/${MODULE_SOURCE_FILES}
        MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/${MODULE_SOURCE_FILES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Copy main module ${MODULE_SOURCE_FILES}"
    )
    list(APPEND MODULE_SOURCE_LINKS ${CMAKE_CURRENT_BINARY_DIR}/${MODULE_SOURCE_FILES})


    add_custom_command(
        OUTPUT ${MODULE_BIN_FILENAME}
        COMMAND ${KBUILD_CMD}
        MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/${MODULE_SOURCE_FILES}
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

    message(STATUS "Deps: ${MODULE_SOURCE_LINKS}")
    add_custom_target(kmod-build ALL DEPENDS ${MODULE_SOURCE_LINKS} ${CMAKE_CURRENT_BINARY_DIR}/${MODULE_FILENAME} )

    install(FILES ${MODULE_BIN_FILENAME} DESTINATION /lib/modules/${KERNEL_RELEASE}/updates/)
endif(USE_KMOD)
