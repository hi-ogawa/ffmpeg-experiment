cmake_minimum_required(VERSION 3.0.0)
project(ffmpeg-emscripten-demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
add_compile_options(-Wall -Wextra)

add_executable(example src/example.cpp)
