
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

set(${PROJECT_NAME}_SRCS
	${PROJECT_NAME}.cpp
	BackgroundManager.cpp
	${PROJECT_NAME}.rdef)

haiku_add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_SRCS})

target_link_libraries(${PROJECT_NAME} be)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
