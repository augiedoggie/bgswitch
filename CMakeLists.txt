cmake_minimum_required(VERSION 3.19)

project(bgswitch)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMakeModules")

include(UseHaiku)

option(STRICT_WARNINGS "Compile with extra warnings and errors" ON)
if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 2.96 AND STRICT_WARNINGS)
	add_compile_options(-Wall -Wextra -Wshadow -Werror)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_definitions(-DDEBUG)
endif(CMAKE_BUILD_TYPE STREQUAL "Debug")

add_subdirectory(Source)
