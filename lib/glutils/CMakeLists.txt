project(glutils)

cmake_minimum_required(VERSION 3.22)

SET(HEADERS
	include/ogl_utils.h
)

SET(CPP
	cpp/ogl_utils.cpp
)

SET(SRC ${HEADERS} ${CPP})

add_library(${PROJECT_NAME} STATIC ${SRC})
target_link_libraries(${PROJECT_NAME} PUBLIC glm glad error_code)

target_include_directories(${PROJECT_NAME} PUBLIC include)