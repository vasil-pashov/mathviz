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

register_shader("assets/shaders/flat_color.glsl" "FlatColor")
register_shader("assets/shaders/morph.glsl" "Morph")
register_shader("assets/shaders/gradient_2d.glsl" "Gradient2D")
create_shader_manifest()

