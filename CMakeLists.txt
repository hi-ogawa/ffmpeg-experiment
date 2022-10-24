cmake_minimum_required(VERSION 3.0.0)
project(ffmpeg-emscripten-demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
add_compile_options(-Wall -Wextra -Wshadow -Wno-missing-braces)

# sanitizer
if (CMAKE_BUILD_TYPE STREQUAL Debug)
  add_compile_options(-fsanitize=address -fsanitize=undefined)
  add_link_options(-fsanitize=address -fsanitize=undefined)
endif()

# ffmpeg
add_library(ffmpeg INTERFACE)
target_link_libraries(ffmpeg INTERFACE -L${CMAKE_BINARY_DIR}/../ffmpeg/prefix/lib -lavformat -lavcodec -lavutil -lswresample)
target_include_directories(ffmpeg INTERFACE ${CMAKE_BINARY_DIR}/../ffmpeg/prefix/include)

# json https://github.com/nlohmann/json
add_library(json INTERFACE)
target_include_directories(ffmpeg INTERFACE ${CMAKE_SOURCE_DIR}/third_party/json/single_include)

# example
add_executable(example-00 src/example-00.cpp)
target_link_libraries(example-00 ffmpeg)

add_executable(example-02 src/example-02.cpp)
target_link_libraries(example-02 ffmpeg)

add_executable(example-03 src/example-03.cpp)
target_link_libraries(example-03 ffmpeg)

add_executable(example-04 src/example-04.cpp)
target_link_libraries(example-04 ffmpeg)

# emscripten
get_filename_component(COMPILER_BASENAME "${CMAKE_C_COMPILER}" NAME)
if (COMPILER_BASENAME STREQUAL emcc)
  add_executable(emscripten-00 src/emscripten-00.cpp)
  target_link_libraries(emscripten-00 PRIVATE ffmpeg json)
  target_link_options(emscripten-00 PRIVATE "SHELL: --bind -s ALLOW_MEMORY_GROWTH=1")
endif()
