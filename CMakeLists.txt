cmake_minimum_required (VERSION 2.6)
project (eoblock)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
SET( CMAKE_EXPORT_COMPILE_COMMANDS ON )

enable_testing()
#include(CTest)

# Find kernel headers
find_package(KernelHeaders REQUIRED)

option (USE_KMOD
	"Buld kmod module" OFF)

set(DEFAULT_CFLAGS "-Wall -std=gnu11 -pedantic -fms-extensions")
set(CMAKE_BUILD_TYPE Debug)

add_subdirectory(cmd) 
add_subdirectory(lib)
add_subdirectory(modules) 

add_subdirectory(tests)
