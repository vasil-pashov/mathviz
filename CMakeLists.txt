project(mathviz)
cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 17)


add_subdirectory(lib/glutils)

add_subdirectory(vendor/glm)
add_subdirectory(vendor/glad)
add_subdirectory(vendor/error-code)
add_subdirectory(vendor/stb)

# Override GLFW build options
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_BUILD_TESTS OFF)
option(GLFW_BUILD_DOCS OFF)
option(GLFW_INSTALL OFF)
add_subdirectory(vendor/glfw)

set(CPP
	cpp/main.cpp
	cpp/primitives.cpp
)
set(HEADERS
	include/primitives.h
)

add_executable(${PROJECT_NAME} ${CPP} ${HEADERS})
target_link_libraries(${PROJECT_NAME} PRIVATE glutils glfw error_code)
target_include_directories(${PROJECT_NAME} PRIVATE include)

# Handle resources
# For now add dependency on all files in the assets folder
# and copy all assets if any of them changes. This might
# become slow when assets grow, but it's the easiest way
# to do it at this point.
file(GLOB_RECURSE ASSETS ${PROJECT_SOURCE_DIR}/assets/*)
add_custom_command(
	OUTPUT assets/assets.stamp
	DEPENDS "${ASSETS}"
	COMMAND ${CMAKE_COMMAND} -E copy_directory
			${CMAKE_SOURCE_DIR}/assets
			$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
	COMMAND ${CMAKE_COMMAND} -E touch assets/assets.stamp
	COMMENT "Copy assets to build directory"
)

add_custom_target(
	prepare_assets ALL
	DEPENDS assets/assets.stamp
)