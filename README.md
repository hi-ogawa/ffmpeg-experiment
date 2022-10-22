# ffmpeg-experiment

```sh
git submodule update --init

# build ffmpeg
bash misc/ffmpeg-configure.sh "$PWD/build/ffmpeg" --prefix="$PWD/build/ffmpeg-prefix" \
  --disable-autodetect --disable-everything --disable-asm --disable-doc \
  --enable-demuxer=webm_dash_manifest \
  --enable-muxer=opus \
  --enable-encoder=opus \
  --enable-decoder=opus
make -C build/ffmpeg
make -C build/ffmpeg install

# build example
cmake -G Ninja . -B build/cc/Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/cc/Debug

# demux/decode (webm/opus -> raw audio)
youtube-dl -f 251 -o test.webm https://www.youtube.com/watch?v=le0BLAEO93g
./build/cc/Debug/example-02 --in test.webm --out test.bin
ffplay -f f32le -ac 1 -ar 48000 test.bin

# emscripten (inside `docker-compose run --rm emscripten`)
# cmake . -B build/js/Debug -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake
# cmake --build build/js/Debug
```

## TODO

- c++ wrapper
  - buffer to pass data between js/ffmpeg
  - mux/codec context management
- emscripten build
  - static compile
  - external library
  - embind
- mux/demux
