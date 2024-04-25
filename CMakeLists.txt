cmake_minimum_required(VERSION 3.19)

project(bgswitch)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMakeModules")

include(UseHaiku)

set(DOCS_DIR "${CMAKE_INSTALL_PREFIX}/documentation/${PROJECT_NAME}" CACHE FILEPATH "Location of documentation")
add_definitions("-DDOCS_DIR=${DOCS_DIR}")

option(BUILD_USERGUIDE "Use sphinx to generate the user guide" ON)
if(BUILD_USERGUIDE)
	find_package(Sphinx)
	if (Sphinx_FOUND)
		add_subdirectory(UserGuide)
	else()
		message(WARNING "Sphinx was not found.  You will not be able to build the user guide.")
	endif()
else()
	message("Generating user guide is disabled.")
endif()

option(USE_CLANG "Enable building with clang instead of gcc" OFF)
if(USE_CLANG)
	set(CMAKE_CXX_COMPILER clang++)
	set(CMAKE_C_COMPILER clang)
	add_compile_options(-fPIC)
endif()

option(STRICT_WARNINGS "Compile with extra warnings and errors" ON)
if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 2.96 AND STRICT_WARNINGS)
	add_compile_options(-Wall -Wextra -Wshadow -Werror)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_definitions(-DDEBUG)
endif(CMAKE_BUILD_TYPE STREQUAL "Debug")

add_subdirectory(Source)
