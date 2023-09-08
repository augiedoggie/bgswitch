cmake_minimum_required(VERSION 3.19)

project(bgswitch)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMakeModules")

include(UseHaiku)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_definitions(-DDEBUG)
endif(CMAKE_BUILD_TYPE STREQUAL "Debug")

add_subdirectory(Source)
