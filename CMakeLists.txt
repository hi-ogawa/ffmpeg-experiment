cmake_minimum_required(VERSION 3.0.0)
project(ffmpeg-emscripten-demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
add_compile_options(-Wall -Wextra)

# ffmpeg (using system installed library for now)
# https://cmake.org/cmake/help/latest/module/FindPkgConfig.html
find_package(PkgConfig REQUIRED)
pkg_check_modules(AVFORMAT libavformat)

# ffmpeg (TODO: probably need to compile everything together for emscripten)
# https://gitlab.kitware.com/cmake/community/-/wikis/doc/tutorials/Exporting-and-Importing-Targets
# add_library(libavformat STATIC IMPORTED)
# set_property(TARGET libavformat PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/build/ffmpeg/libavformat/libavformat.a)
# target_link_libraries(libavformat INTERFACE m atomic bz2 z)

# add_library(ffmpeg)
# build/ffmpeg-prefix/include
# build/ffmpeg-prefix/lib/pkgconfig/libavformat.pc

# main
add_executable(example src/example.cpp)
target_link_libraries(example ${AVFORMAT_LIBRARIES})
target_include_directories(example PRIVATE ${AVFORMAT_INCLUDE_DIRS})

# target_compile_options(testapp PUBLIC ${AVFORMAT_CFLAGS_OTHER})
# target_link_libraries(example AVF)
# target_link_libraries(example libavformat)
# target_link_options(example PRIVATE -L${CMAKE_CURRENT_SOURCE_DIR}/build/ffmpeg-prefix/lib -lavformat -lm -latomic -lbz2 -lz)
# target_link_options(example PRIVATE "-lavformat")
# target_link_libraries(example INTERFACE "-L ${CMAKE_CURRENT_SOURCE_DIR}/build/ffmpeg-prefix/lib -lavformat")

# target_link_libraries(example ${CMAKE_CURRENT_SOURCE_DIR}/build/ffmpeg/libavformat/libavformat.a)
# target_link_libraries(example ${CMAKE_CURRENT_SOURCE_DIR}/build/ffmpeg-prefix/lib/libavformat.a)
# target_include_directories(example ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/FFmpeg)
# target_link_libraries(example PRIVATE m atomic bz2 z)

# -lavformat -lm -latomic -lbz2 -lz
# build/ffmpeg-prefix/lib/libavformat.a

# -L/home/savio/opencv-3.0.0/cmake_binary_dir/lib
