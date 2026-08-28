cmake_minimum_required(VERSION 3.16)
project(software-rasterizer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(software-rasterizer
  main.cpp
  tgaimage.cpp
)

target_include_directories(software-rasterizer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
