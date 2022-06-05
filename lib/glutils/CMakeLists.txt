project(glutils)

cmake_minimum_required(VERSION 3.22)

SET(HEADERS
	include/glutils.h
)

SET(CPP
	cpp/glutils.cpp
)

SET(SRC ${HEADERS} ${CPP})

add_library(${PROJECT_NAME} STATIC ${SRC})
target_link_libraries(${PROJECT_NAME} PUBLIC glm glad error_code stb)

target_include_directories(${PROJECT_NAME} PUBLIC include)