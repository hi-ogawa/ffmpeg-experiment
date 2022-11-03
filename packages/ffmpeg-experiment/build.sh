#!/bin/bash
set -eu -o pipefail

this_dir=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)
cd "$this_dir"

rm -rf build
mkdir -p build

cp -f ../../build/emscripten/Release/emscripten-01.{js,wasm} build
cp -f ../../build/emscripten/ffmpeg-tools/ffmpeg_g.{js,wasm,worker.js} build
