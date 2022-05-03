project(mathviz)
cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 17)


add_subdirectory(lib/glutils)

add_subdirectory(vendor/glm)
add_subdirectory(vendor/glad)
add_subdirectory(vendor/error_code)

# Override GLFW build options
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_BUILD_TESTS OFF)
option(GLFW_BUILD_DOCS OFF)
option(GLFW_INSTALL OFF)
add_subdirectory(vendor/glfw)

set(CPP cpp/main.cpp)

add_executable(${PROJECT_NAME} ${CPP})
target_link_libraries(${PROJECT_NAME} PRIVATE glutils glfw error_code)

