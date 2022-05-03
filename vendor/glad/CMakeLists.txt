project(glad)
cmake_minimum_required(VERSION 3.22)

add_library(${PROJECT_NAME} STATIC src/glad.c)
target_include_directories(${PROJECT_NAME} PUBLIC include)