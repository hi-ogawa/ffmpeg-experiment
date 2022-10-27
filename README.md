# ffmpeg-experiment

## goals

- using ffmpeg from c++
- ffmpeg emscripten build
- simple examples
  - metadata manipulation
  - demux, mux, decode, encode
  - in-memory buffer for IO without file system

## todo

- test on browser
- create npm package
- mux opus and vp9 into single webm
- static link ffmpeg external library (e.g. mp4)

```sh
git submodule update --init

#
# normal build to get familar with ffmpeg api
#

# build ffmpeg
bash misc/ffmpeg-configure.sh "$PWD/build/native/ffmpeg" --prefix="$PWD/build/native/ffmpeg/prefix" \
  --disable-autodetect --disable-everything --disable-asm --disable-doc \
  --enable-protocol=file \
  --enable-demuxer=webm_dash_manifest,ogg \
  --enable-muxer=opus \
  --enable-encoder=opus \
  --enable-decoder=opus
make -j -C build/native/ffmpeg
make -C build/native/ffmpeg install

# build example
cmake -G Ninja . -B build/native/Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/native/Debug
ldd ./build/native/Debug/example-00 # verify ffmepg is linked statically

# download test files (webm and jpg)
youtube-dl -f 251 -o test.webm https://www.youtube.com/watch?v=le0BLAEO93g
wget -O test.jpg https://i.ytimg.com/vi/le0BLAEO93g/maxresdefault.jpg

# print metadata
./build/native/Debug/example-00 --in test.webm

# demux/decode (webm -> raw audio)
./build/native/Debug/example-02 --in test.webm --out test.bin
ffplay -f f32le -ac 1 -ar 48000 test.bin

# extract audio (webm -> opus)
./build/native/Debug/example-03 --in test.webm --out test.opus
ffmpeg -i test.webm -c copy test.reference.opus  # compare with ffmpeg

# extract audio and embed metadata and cover art
./build/native/Debug/example-03 --in test.webm --out test.opus --in-metadata '{ "title": "Dean Town", "artist": "Vulfpeck" }' --in-picture test.jpg

# transcode (webm -> opus) (TODO: not working. could be due to experimental ffmpeg's experimental opus encoder)
./build/native/Debug/example-04 --in test.webm --out test.opus

#
# emscripten build (run inside `docker-compose run --rm emscripten`)
#

# https://github.com/FFmpeg/FFmpeg/blob/81bc4ef14292f77b7dcea01b00e6f2ec1aea4b32/configure#L371-L401
# https://github.com/emscripten-core/emscripten/blob/00daf403cd09467c6dab841b873710bb878535b2/tools/building.py#L63-L83
bash misc/ffmpeg-configure.sh "/app/build/emscripten/ffmpeg" --prefix="/app/build/emscripten/ffmpeg/prefix" \
  --enable-cross-compile \
  --cc=/emsdk/upstream/emscripten/emcc \
  --cxx=/emsdk/upstream/emscripten/em++ \
  --ar=/emsdk/upstream/emscripten/emar \
  --ld=/emsdk/upstream/emscripten/emcc \
  --nm=/emsdk/upstream/bin/llvm-nm \
  --ranlib=/emsdk/upstream/emscripten/emranlib \
  --disable-autodetect --disable-everything --disable-asm --disable-doc --disable-programs \
  --enable-demuxer=webm_dash_manifest,ogg \
  --enable-muxer=opus \
  --enable-encoder=opus \
  --enable-decoder=opus
make -j -C build/emscripten/ffmpeg
make -C build/emscripten/ffmpeg install

cmake . -B build/emscripten/Debug -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake
cmake --build build/emscripten/Debug
# demo js code should be run outside of `docker-compose run`
node ./src/emscripten-00-demo.js ./build/emscripten/Debug/emscripten-00.js test.webm
node ./src/emscripten-01-demo.js --module ./build/emscripten/Debug/emscripten-01.js --in test.webm --out test.opus --in-picture test.jpg --in-metadata '{ "title": "Dean Town", "artist": "Vulfpeck" }'

# optimized build
cmake . -B build/emscripten/Release -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake
cmake --build build/emscripten/Release
node ./src/emscripten-00-demo.js ./build/emscripten/Release/emscripten-00.js test.webm
node ./src/emscripten-01-demo.js --module ./build/emscripten/Release/emscripten-01.js --in test.webm --out test.opus --in-picture test.jpg --in-metadata '{ "title": "Dean Town", "artist": "Vulfpeck" }'
```
