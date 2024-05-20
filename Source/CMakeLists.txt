
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})


option(BUILD_BGSWITCH "Build the bgswitch command line app" ON)
if(BUILD_BGSWITCH)
	set(${PROJECT_NAME}_SRCS
		${PROJECT_NAME}.cpp
		BackgroundManager.cpp
		${PROJECT_NAME}.rdef)

	haiku_add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_SRCS})

	target_link_libraries(${PROJECT_NAME} be)

	install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
endif()


option(BUILD_WALLRUS "Build the Wallrus rotation app" OFF)
if(BUILD_WALLRUS)
	set(WALLRUS_BIN "${CMAKE_INSTALL_PREFIX}/servers/Wallrus")
	configure_file(wallrus.launch.in ${PROJECT_BINARY_DIR}/wallrus.launch)
	set(LAUNCH_PREFIX /boot/system/non-packaged/data/user_launch/ CACHE PATH "Install prefix for launch_roster files")
	install(FILES ${CMAKE_BINARY_DIR}/wallrus.launch DESTINATION ${LAUNCH_PREFIX})

	configure_file(${CMAKE_MODULE_PATH}/CopySettings.cmake.in ${PROJECT_BINARY_DIR}/CMakeFiles/CopySettings.cmake @ONLY)

	install(SCRIPT ${PROJECT_BINARY_DIR}/CMakeFiles/CopySettings.cmake)

	execute_process(
		COMMAND finddir B_SYSTEM_HEADERS_DIRECTORY
		OUTPUT_VARIABLE B_SYSTEM_HEADERS_DIRECTORY
		OUTPUT_STRIP_TRAILING_WHITESPACE)

	include_directories(
		"${B_SYSTEM_HEADERS_DIRECTORY}/private"
		"${B_SYSTEM_HEADERS_DIRECTORY}/private/shared")

	set(Wallrus_SRCS
		WallrusApp.cpp
		WallrusAppScripting.cpp
		BackgroundManager.cpp
		Wallrus.rdef)

	haiku_add_executable(Wallrus ${Wallrus_SRCS})

	target_link_libraries(Wallrus be shared)

	install(TARGETS Wallrus RUNTIME DESTINATION servers)
endif()
