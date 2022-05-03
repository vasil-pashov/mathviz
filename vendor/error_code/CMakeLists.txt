cmake_minimum_required(VERSION 3.14.0)
project(error_code
	VERSION 0.1.0
	LANGUAGES CXX
)

add_library(${PROJECT_NAME} STATIC
	cpp/error_code.cpp
	include/error_code.h
)

target_include_directories(${PROJECT_NAME} PUBLIC include)

