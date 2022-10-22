cmake_minimum_required(VERSION 3.0.0)
project(ffmpeg-emscripten-demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
add_compile_options(-Wall -Wextra -Wshadow -Wno-missing-braces)
add_compile_options(-fsanitize=address -fsanitize=undefined)
add_link_options(-fsanitize=address -fsanitize=undefined)

# ffmpeg
add_library(ffmpeg INTERFACE)
target_link_libraries(ffmpeg INTERFACE -L${CMAKE_CURRENT_SOURCE_DIR}/build/ffmpeg-prefix/lib -lavformat -lavcodec -lavutil)
target_include_directories(ffmpeg INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/build/ffmpeg-prefix/include)

# example
add_executable(example src/example.cpp)
target_link_libraries(example ffmpeg)

add_executable(example-02 src/example-02.cpp)
target_link_libraries(example-02 ffmpeg)
