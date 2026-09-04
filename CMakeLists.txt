cmake_minimum_required(VERSION 3.16)
project(software-renderer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Homebrew SDL2 (sdl2-compat) on Apple Silicon
if(APPLE AND EXISTS "/opt/homebrew/opt/sdl2-compat")
  list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/sdl2-compat")
endif()

find_package(SDL2 REQUIRED)

add_executable(software-renderer
  src/main.cpp
  src/pipeline.cpp
  src/tgaimage.cpp
)

target_include_directories(software-renderer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(software-renderer PRIVATE SDL2::SDL2)
