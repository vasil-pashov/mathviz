project(mathviz)
cmake_minimum_required(VERSION 3.24)

include(FetchContent)
include(ExternalProject)


set(CMAKE_CXX_STANDARD 17)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

FetchContent_Declare(
	error-code
	GIT_REPOSITORY "https://github.com/vasil-pashov/error-code"
	GIT_TAG "v0.1.1"
)

FetchContent_Declare(
	glm
	GIT_REPOSITORY "https://github.com/g-truc/glm"
	GIT_TAG "0.9.9.8"
)

option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_BUILD_TESTS OFF)
option(GLFW_BUILD_DOCS OFF)
option(GLFW_INSTALL OFF)
FetchContent_Declare(
	glfw
	GIT_REPOSITORY "https://github.com/glfw/glfw"
	GIT_TAG "3.3.8"
)

# Unfortunately STB does is not a CMake Project. So we would have to populate it manually
# with FetchContent_Populate instead of using FetchContent_MakeAvailable. This will allow
# to copy a custom CMakeLists.txt into the source dir of stb before running add_subdirectory.
# 1. Declare the STB repo paths
FetchContent_Declare(
	stb
	GIT_REPOSITORY https://github.com/nothings/stb.git
	GIT_TAG af1a5bc352164740c1cc1354942b1c6b72eacb8a
)
# 2. Populate it. This will trigger the download and will also make available stb_SOURCE_DIR
# variable in CMake
FetchContent_Populate(stb)
# 3. Copy the custom CMakeLists.txt into the STB source directory
file(COPY_FILE ${CMAKE_SOURCE_DIR}/cmake/stb/CMakeLists.txt ${stb_SOURCE_DIR}/CMakeLists.txt)
# 4. Add stb subdir. This will trigger the build
add_subdirectory(${stb_SOURCE_DIR})

# As per CMake official docs advice FetchContent_MakeAvailable should be called after
# all calls to FetchContent_Declare. Do not include stb here as it was manually populated.
FetchContent_MakeAvailable(
	error-code
	glm
	glfw
)

add_subdirectory(vendor/glad)
add_subdirectory(lib/glutils)

set(CPP
	cpp/main.cpp
	cpp/geometry_primitives.cpp
	cpp/context.cpp
	cpp/material.cpp
)
set(HEADERS
	include/geometry_primitives.h
	include/context.h
	include/material.h
)

set(GLOBAL_SHADER_PATHS)
set(GLOBAL_SHADER_ENUMS)
function(register_shader shader_path enum_name)
	list(APPEND GLOBAL_SHADER_PATHS ${shader_path})
	set(GLOBAL_SHADER_PATHS ${GLOBAL_SHADER_PATHS} PARENT_SCOPE)

	list(APPEND GLOBAL_SHADER_ENUMS ${enum_name})
	set(GLOBAL_SHADER_ENUMS ${GLOBAL_SHADER_ENUMS} PARENT_SCOPE)
endfunction()

# Create the CPP header file describing all shaders registered via register_shader
# The file contains an array of relative paths to the shaders and an enum used to
# index into the array. This must be called after the main project target was defied
# (after the call to add_executable(${PROJECT_NAME}))
function(create_shader_manifest)
	set(first_enum TRUE)
	set(shader_table_enum "enum class ShaderTable {\n")
	foreach(shader_enum ${GLOBAL_SHADER_ENUMS})
		if(${first_enum})
			string(APPEND shader_table_enum "\t\t${shader_enum} = 0,\n")
			set(first_enum FALSE)
		else()
			string(APPEND shader_table_enum "\t\t${shader_enum},\n")
		endif()
	endforeach()
	string(APPEND shader_table_enum "\t\tCount\n\t};\n")

	set(shader_paths_array "inline const char* shaderPaths[int(ShaderTable::Count)] = {\n")
	foreach(shader_path ${GLOBAL_SHADER_PATHS})
		string(APPEND shader_paths_array "\t\t\"${shader_path}\",\n")
	endforeach()
	string(APPEND shader_paths_array "\t};\n")
	configure_file(cmake/template/shader_bindings.h.in generated/include/shader_bindings.h @ONLY)
	target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/generated/include)
endfunction()

# Shader registry must happen before the target is created so that shader files are added to the
# target. This enables to create a virtual folder in the IDE where we can see all shaders.
register_shader("assets/shaders/flat_color.glsl" "FlatColor")
register_shader("assets/shaders/morph.glsl" "Morph")
register_shader("assets/shaders/gradient_2d.glsl" "Gradient2D")

add_executable(${PROJECT_NAME} ${CPP} ${HEADERS} ${GLOBAL_SHADER_PATHS})
target_link_libraries(${PROJECT_NAME} PRIVATE glutils glfw error_code)
target_include_directories(${PROJECT_NAME} PRIVATE include)

# Handle resources
# For now add dependency on all files in the assets folder
# and copy all assets if any of them changes. This might
# become slow when assets grow, but it's the easiest way
# to do it at this point.
file(GLOB_RECURSE ASSETS ${PROJECT_SOURCE_DIR}/assets/*)
add_custom_command(
	OUTPUT assets.stamp
	DEPENDS "${ASSETS}"
	COMMAND ${CMAKE_COMMAND} -E copy_directory
			${CMAKE_SOURCE_DIR}/assets
			${CMAKE_CURRENT_BINARY_DIR}/assets
	COMMAND ${CMAKE_COMMAND} -E touch assets.stamp
	COMMENT "Copy assets to build directory ${CMAKE_CURRENT_BINARY_DIR}/assets"
)

add_custom_target(
	prepare_assets ALL
	DEPENDS assets.stamp
)

if(MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE
		/W4
		/Wall
		/we4834 # [[nodiscard]] attribute with ignored result
		/we4715 # Not all control paths return a value
		/we5038 # Constructor initialization order doe not match order of declaration
	)
else()
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Add all shaders to the virtual IDE folder with the shaders. For some reason adding them with one call like this:
# 	source_group(
#		TREE "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders"
#		PREFIX "Shaders"
#		FILES "${GLOBAL_SHADER_PATHS}"
#	)
# does not work, so we add them one by one.
foreach(shader_path ${GLOBAL_SHADER_PATHS})
	source_group(
		TREE "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders"
		PREFIX "Shaders"
		FILES "${shader_path}"
	)
endforeach()

create_shader_manifest()
