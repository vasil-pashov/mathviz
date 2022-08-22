cmake_minimum_required(VERSION 3.14.5)
project(imgui LANGUAGES CXX)

# @set OUT_DIR=Debug
# @set OUT_EXE=example_glfw_opengl3
# @set INCLUDES=/I..\.. /I..\..\backends /I..\libs\glfw\include
# @set SOURCES=main.cpp ..\..\backends\imgui_impl_glfw.cpp ..\..\backends\imgui_impl_opengl3.cpp ..\..\imgui*.cpp
# @set LIBS=/LIBPATH:..\libs\glfw\lib-vc2010-32 glfw3.lib opengl32.lib gdi32.lib shell32.lib
# mkdir %OUT_DIR%
# cl /nologo /Zi /MD %INCLUDES% %SOURCES% /Fe%OUT_DIR%/%OUT_EXE%.exe /Fo%OUT_DIR%/ /link %LIBS%

set(BACKEND_SOURCES
	backends/imgui_impl_glfw.cpp
	backends/imgui_impl_opengl3.cpp
)

file(GLOB IMGUI_SOURCES imgui*.cpp)

add_library(${PROJECT_NAME} STATIC ${BACKEND_SOURCES} ${IMGUI_SOURCES})
target_include_directories(${PROJECT_NAME}
	PUBLIC
		backends
		${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(${PROJECT_NAME} PUBLIC glfw)